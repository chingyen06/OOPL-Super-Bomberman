#include "AIManager.hpp"
#include "GameConstants.hpp"
#include "GameTypes.hpp"
#include "GridCoord.hpp"
#include "Interactable.hpp"
#include "Spirit.hpp"
#include "Turret/TurretManager.hpp"
#include "Controller/BotController.hpp"
#include <queue>
#include <cmath>
#include <algorithm>
#include <functional>

struct AStarNode {
    int x, y;
    int g, h;
    std::shared_ptr<AStarNode> parent;
    int f() const { return g + h; }
};

struct CompareNode {
    bool operator()(const std::shared_ptr<AStarNode>& a, const std::shared_ptr<AStarNode>& b) {
        return a->f() > b->f();
    }
};

std::vector<std::pair<int, int>> AIManager::FindPath(int startX, int startY, int targetX, int targetY, int mapW, int mapH, std::function<int(int, int)> costFunc) {
    std::vector<std::pair<int, int>> path;
    if (startX == targetX && startY == targetY) return path;

    std::vector<std::vector<bool>> closedList(mapH, std::vector<bool>(mapW, false));
    std::priority_queue<std::shared_ptr<AStarNode>, std::vector<std::shared_ptr<AStarNode>>, CompareNode> openList;

    int startH = std::abs(startX - targetX) + std::abs(startY - targetY);
    openList.push(std::make_shared<AStarNode>(AStarNode{ startX, startY, 0, startH, nullptr }));

    while (!openList.empty()) {
        auto current = openList.top();
        openList.pop();

        if (closedList[current->y][current->x]) continue;
        closedList[current->y][current->x] = true;

        if (current->x == targetX && current->y == targetY) {
            auto currNode = current;
            while (currNode->parent != nullptr) {
                path.push_back({ currNode->x, currNode->y });
                currNode = currNode->parent;
            }
            std::reverse(path.begin(), path.end());
            return path;
        }

        for (const auto& off : kCardinalOffsets) {
            int nx = current->x + off.dx;
            int ny = current->y + off.dy;

            if (nx >= 0 && nx < mapW && ny >= 0 && ny < mapH) {
                int cost = costFunc(nx, ny);
                if (cost >= 0 && !closedList[ny][nx]) {
                    int g = current->g + cost;
                    int h = std::abs(nx - targetX) + std::abs(ny - targetY);
                    openList.push(std::make_shared<AStarNode>(AStarNode{ nx, ny, g, h, current }));
                }
            }
        }
    }
    return path;
}

void AIManager::RebuildDangerMap(const LevelManager& levelManager, const BombManager& bombManager) {
    int mapW = levelManager.GetMapWidth();
    int mapH = levelManager.GetMapHeight();
    m_Danger.assign(mapH, std::vector<bool>(mapW, false));

    // 1) 已存在的爆炸火焰
    for (int y = 0; y < mapH; ++y) {
        for (int x = 0; x < mapW; ++x) {
            if (bombManager.HasExplosionAt(x, y)) {
                m_Danger[y][x] = true;
            }
        }
    }

    // 2) 倒數中的炸彈會掃到的格子 (沿四方向延伸到第一個牆/磚為止)
    for (int by = 0; by < mapH; ++by) {
        for (int bx = 0; bx < mapW; ++bx) {
            if (!bombManager.IsBombAt(bx, by)) continue;

            int bfp = bombManager.GetFirepowerAt(bx, by);
            m_Danger[by][bx] = true;

            for (const auto& off : kCardinalOffsets) {
                for (int step = 1; step <= bfp; ++step) {
                    int cx = bx + off.dx * step;
                    int cy = by + off.dy * step;
                    if (cx < 0 || cx >= mapW || cy < 0 || cy >= mapH) break;
                    m_Danger[cy][cx] = true;
                    if (!levelManager.IsWalkable(cx, cy)) break;
                }
            }
        }
    }
}

void AIManager::Update(std::vector<std::shared_ptr<Player>>& players,
    const LevelManager& levelManager,
    const BombManager& bombManager,
    const InteractableManager& interactableManager,
    const std::vector<std::shared_ptr<Spirit>>& spirits,
    const TurretManager& turretManager) {

    RebuildDangerMap(levelManager, bombManager);

    const auto& interactables = interactableManager.GetInteractables();
    int mapW = levelManager.GetMapWidth();
    int mapH = levelManager.GetMapHeight();

    // 跨 bot 共享的目標鎖定表：本幀內某物件被某 bot 鎖定後，其他 bot 不再追同一物
    std::unordered_set<Interactable*> claimedTargets;

    // 把已決定要放的「pending 炸彈」直接寫入 m_Danger，讓後續 bot 的 IsLethal/FindSafeSpot
    // 自動納入考量 — 防止多隻 bot 各自以為安全、放完發現連鎖把全員炸死
    auto registerPendingBomb = [&](int bx, int by, int fp) {
        if (bx < 0 || by < 0 || by >= static_cast<int>(m_Danger.size()) ||
            bx >= static_cast<int>(m_Danger[0].size())) return;
        m_Danger[by][bx] = true;
        for (const auto& off : kCardinalOffsets) {
            for (int step = 1; step <= fp; ++step) {
                int cx = bx + off.dx * step;
                int cy = by + off.dy * step;
                if (cx < 0 || cy < 0 || cy >= static_cast<int>(m_Danger.size()) ||
                    cx >= static_cast<int>(m_Danger[0].size())) break;
                m_Danger[cy][cx] = true;
                if (!levelManager.IsWalkable(cx, cy)) break;
            }
        }
    };

    for (auto& bot : players) {
        if (bot->IsDead()) continue;
        auto botController = dynamic_cast<BotController*>(bot->GetController());
        if (!botController) continue;

        botController->TickCooldown();

        int botX = bot->GetGridX();
        int botY = bot->GetGridY();
        int botFirepower = bot->GetFirepower();

        // 反應延遲：cooldown 中跳過決策，input 保持上一次設定。
        // 下列情況必須 bypass、每幀重決策：
        //  (a) 身處危險 — 100ms 反應太慢可能直接被炸死
        //  (b) 站在輸送帶上 — 力場每幀都在推，需要即時調整方向鍵
        //  (c) bot 像素位置已偏離當前格中心 (>2 px) — 必須每幀微修否則就 S 型走
        const bool inDanger = IsLethal(botX, botY, levelManager, botFirepower);
        const auto forceHere = interactableManager.GetForceAt(botX, botY);
        const bool onForceField = (forceHere.x != 0.0f || forceHere.y != 0.0f);

        const auto botPixel = bot->GetPixelPos();
        const float curCenterX = GridCoord::ToPixelX(botX);
        const float curCenterY = GridCoord::ToPixelY(botY);
        const bool needsAlignment = std::abs(botPixel.x - curCenterX) > 2.0f
                                 || std::abs(botPixel.y - curCenterY) > 2.0f;

        const bool needsImmediate = inDanger || onForceField || needsAlignment;
        if (!needsImmediate && !botController->IsReadyToDecide()) continue;

        // 把其他活著的 player當作 soft obstacle，避免多隻 AI 走在同一條路擠在一格放不下炸彈
        auto isBlockedByOther = [&](int x, int y) {
            for (const auto& other : players) {
                if (other.get() == bot.get()) continue;
                if (other->IsDead()) continue;
                if (other->GetGridX() == x && other->GetGridY() == y) return true;
            }
            return false;
        };
        constexpr int kOtherPlayerPenalty = 25;  // 走過對方所在格成本很高，強烈傾向繞路

        // 源石精靈所在格 = 碰到立即被殺 (對攻擊方)，當成 hard obstacle 排除
        auto isSpiritAt = [&](int x, int y) {
            for (const auto& s : spirits) {
                if (s->ShouldDelete()) continue;
                if (s->GetGridX() == x && s->GetGridY() == y) return true;
            }
            return false;
        };

        // 砲台所在格不能走 (撞上去就卡住)
        auto isTurretAt = [&](int x, int y) {
            return turretManager.IsTurretAt(x, y);
        };

        // 判斷在 (bx,by) 放炸彈能否炸到任何源石精靈 (用於主動清怪)
        auto willHitAnySpirit = [&](int bx, int by, int fp) {
            for (const auto& s : spirits) {
                if (s->ShouldDelete()) continue;
                int sx = s->GetGridX(), sy = s->GetGridY();
                if (bx == sx && by == sy) return true;
                for (const auto& off : kCardinalOffsets) {
                    for (int step = 1; step <= fp; step++) {
                        int cx = bx + off.dx * step;
                        int cy = by + off.dy * step;
                        if (cx == sx && cy == sy) return true;
                        if (!levelManager.IsWalkable(cx, cy)) break;
                    }
                }
            }
            return false;
        };

        // 決策完成後設定下次冷卻 (用 playerID 做相位偏移，分散不同 bot 的決策幀)
        // immediate 情境 (危險 / 輸送帶 / 像素偏離) cooldown = 0，每幀都會繼續微調 — 避免 S 型走
        struct ResetGuard {
            BotController* ctrl;
            int frames;
            ~ResetGuard() { ctrl->ResetCooldown(frames); }
        };
        const int nextCooldown = needsImmediate
            ? 0
            : Constants::Bot::kReactionFrames + (bot->GetPlayerID() % 3);
        ResetGuard guard{ botController, nextCooldown };

        if (inDanger) {
            auto safeSpot = FindSafeSpot(botX, botY, levelManager, bombManager, botFirepower);
            if (safeSpot.found) {
                auto path = FindPath(botX, botY, safeSpot.x, safeSpot.y, mapW, mapH, [&](int x, int y) {
                    if (!levelManager.IsWalkable(x, y) || bombManager.IsBombAt(x, y) || bombManager.HasExplosionAt(x, y)) return -1;
                    if (isSpiritAt(x, y) || isTurretAt(x, y)) return -1;  // 死亡格 / 撞砲台
                    return isBlockedByOther(x, y) ? kOtherPlayerPenalty : 1;
                    });
                if (!path.empty()) ExecuteMove(botController, botX, botY, path[0].first, path[0].second, false,
                                                bot->GetPixelPos());
            }
            continue;
        }

        std::shared_ptr<Player> targetDefender = nullptr;
        int defenderDist = 999;
        for (const auto& p : players) {
            if (p->GetTeam() == Team::DEFENDER && !p->IsDead()) {
                int dist = std::abs(p->GetGridX() - botX) + std::abs(p->GetGridY() - botY);
                if (dist < defenderDist) {
                    defenderDist = dist;
                    targetDefender = p;
                }
            }
        }

        int targetX = -1, targetY = -1;
        auto nearestTarget = FindNearestTarget(botX, botY, bot->HasKey(), interactables, claimedTargets);

        if (nearestTarget) {
            targetX = nearestTarget->GetGridX();
            targetY = nearestTarget->GetGridY();
            claimedTargets.insert(nearestTarget.get());  // 鎖定，後續 bot 不重複追
        } else if (targetDefender) {
            targetX = targetDefender->GetGridX();
            targetY = targetDefender->GetGridY();
        }

        if (targetX == -1) {
            botController->SetInput(false, false, false, false, false);
            continue;
        }

        // 策略 3：尋找安全的目標
        auto pathSafe = FindPath(botX, botY, targetX, targetY, mapW, mapH, [&](int x, int y) {
            if (!levelManager.IsWalkable(x, y) || bombManager.IsBombAt(x, y) || bombManager.HasExplosionAt(x, y)) return -1;
            if (IsLethal(x, y, levelManager, botFirepower)) return -1;
            if (isSpiritAt(x, y) || isTurretAt(x, y)) return -1;
            return isBlockedByOther(x, y) ? kOtherPlayerPenalty : 1;
            });

        if (!pathSafe.empty()) {
            // 「想要放炸彈」的觸發條件：擊中 defender 或精靈
            bool wantBomb = false;
            if (targetDefender && targetX == targetDefender->GetGridX() && targetY == targetDefender->GetGridY()) {
                auto willHitTarget = [&](int bx, int by, int tx, int ty, int fp) {
                    if (bx == tx && by == ty) return true;
                    for (const auto& off : kCardinalOffsets) {
                        for (int step = 1; step <= fp; step++) {
                            int cx = bx + off.dx * step;
                            int cy = by + off.dy * step;
                            if (cx == tx && cy == ty) return true;
                            if (!levelManager.IsWalkable(cx, cy)) break;
                        }
                    }
                    return false;
                };
                if (willHitTarget(botX, botY, targetDefender->GetGridX(), targetDefender->GetGridY(), botFirepower)) {
                    wantBomb = true;
                }
            }
            if (!wantBomb && willHitAnySpirit(botX, botY, botFirepower)) {
                wantBomb = true;
            }

            // 真正放彈前：確認「逃生路徑」(不只 safe spot 存在，還要走得到)
            bool placeBomb = false;
            int moveToX = pathSafe[0].first;
            int moveToY = pathSafe[0].second;
            if (wantBomb) {
                auto testSafe = FindSafeSpot(botX, botY, levelManager, bombManager, botFirepower, botX, botY);
                if (testSafe.found) {
                    // 注意：escape path 只避開「已存在」的炸彈/爆炸，不避開 pretend bomb 的火力範圍。
                    // 因為剛放的炸彈有 3 秒引信，bot 有充足時間從尚未爆炸的格子穿出去 (testSafe.dist ≤ 5)。
                    auto escapePath = FindPath(botX, botY, testSafe.x, testSafe.y, mapW, mapH, [&](int x, int y) {
                        if (!levelManager.IsWalkable(x, y) || bombManager.IsBombAt(x, y) || bombManager.HasExplosionAt(x, y)) return -1;
                        if (IsLethal(x, y, levelManager, botFirepower)) return -1;  // 真實炸彈/爆炸才擋
                        if (isSpiritAt(x, y) || isTurretAt(x, y)) return -1;
                        return isBlockedByOther(x, y) ? kOtherPlayerPenalty : 1;
                        });
                    if (!escapePath.empty()) {
                        placeBomb = true;
                        // 放彈後優先走「逃生點」而非原本的 target — 否則自己走進爆炸圈
                        moveToX = escapePath[0].first;
                        moveToY = escapePath[0].second;
                        // 通知後續 bot：這顆炸彈的爆炸範圍他們也要避開
                        registerPendingBomb(botX, botY, botFirepower);
                    }
                }
            }

            ExecuteMove(botController, botX, botY, moveToX, moveToY, placeBomb, bot->GetPixelPos());
            continue;
        }

        // 策略 4：無安全路徑 - 嘗試炸牆 (不管爆炸)
        auto pathThroughBricks = FindPath(botX, botY, targetX, targetY, mapW, mapH, [&](int x, int y) {
            if (!levelManager.IsWalkable(x, y) && !levelManager.IsBrick(x, y)) return -1;
            if (bombManager.IsBombAt(x, y) || bombManager.HasExplosionAt(x, y) || IsLethal(x, y, levelManager, botFirepower)) return -1;
            if (isSpiritAt(x, y) || isTurretAt(x, y)) return -1;
            if (levelManager.IsBrick(x, y)) return 50;
            return isBlockedByOther(x, y) ? kOtherPlayerPenalty : 1;
            });

        if (!pathThroughBricks.empty()) {
            int walkToX = botX, walkToY = botY;
            for (auto& p : pathThroughBricks) {
                if (levelManager.IsBrick(p.first, p.second)) break;
                walkToX = p.first; walkToY = p.second;
            }

            if (botX == walkToX && botY == walkToY) {
                // 站在 brick 前準備炸 — 同樣要驗證「逃生路徑」可走得到、且放彈後朝逃生點走
                auto testSafe = FindSafeSpot(botX, botY, levelManager, bombManager, botFirepower, botX, botY);
                bool canBomb = false;
                int moveToX = botX, moveToY = botY;
                if (testSafe.found) {
                    auto escapePath = FindPath(botX, botY, testSafe.x, testSafe.y, mapW, mapH, [&](int x, int y) {
                        if (!levelManager.IsWalkable(x, y) || bombManager.IsBombAt(x, y) || bombManager.HasExplosionAt(x, y)) return -1;
                        if (IsLethal(x, y, levelManager, botFirepower)) return -1;  // pretend bomb 還沒爆，不擋 path
                        if (isSpiritAt(x, y) || isTurretAt(x, y)) return -1;
                        return isBlockedByOther(x, y) ? kOtherPlayerPenalty : 1;
                        });
                    if (!escapePath.empty()) {
                        canBomb = true;
                        moveToX = escapePath[0].first;
                        moveToY = escapePath[0].second;
                        registerPendingBomb(botX, botY, botFirepower);
                    }
                }
                if (canBomb) ExecuteMove(botController, botX, botY, moveToX, moveToY, true, bot->GetPixelPos());
                else         botController->SetInput(false, false, false, false, false);
            }
            else {
                auto pathToBrick = FindPath(botX, botY, walkToX, walkToY, mapW, mapH, [&](int x, int y) {
                    if (!levelManager.IsWalkable(x, y) || bombManager.IsBombAt(x, y) || bombManager.HasExplosionAt(x, y) || IsLethal(x, y, levelManager, botFirepower)) return -1;
                    if (isSpiritAt(x, y) || isTurretAt(x, y)) return -1;
                    return isBlockedByOther(x, y) ? kOtherPlayerPenalty : 1;
                    });
                if (!pathToBrick.empty()) ExecuteMove(botController, botX, botY, pathToBrick[0].first, pathToBrick[0].second, false,
                                                       bot->GetPixelPos());
                else botController->SetInput(false, false, false, false, false);
            }
            continue;
        }

        // 策略 5：無安全路徑 - 無視火焰與障礙 (自殺攻擊)
        // 砲台仍是物理碰撞，自殺也穿不過
        auto pathIgnoreFire = FindPath(botX, botY, targetX, targetY, mapW, mapH, [&](int x, int y) {
            if (!levelManager.IsWalkable(x, y) || isTurretAt(x, y)) return -1;
            return 1;
            });

        if (!pathIgnoreFire.empty()) {
            bool placeBomb = false;

            if (targetDefender) {
                auto hasLineOfSight = [&](int bx, int by, int tx, int ty) {
                    if (bx != tx && by != ty) return false;
                    int stepX = (tx > bx) ? 1 : (tx < bx) ? -1 : 0;
                    int stepY = (ty > by) ? 1 : (ty < by) ? -1 : 0;
                    int cx = bx + stepX, cy = by + stepY;
                    while (cx != tx || cy != ty) {
                        if (!levelManager.IsWalkable(cx, cy)) return false;
                        cx += stepX; cy += stepY;
                    }
                    return true;
                };

                if (defenderDist <= 5 && hasLineOfSight(botX, botY, targetDefender->GetGridX(), targetDefender->GetGridY())) {
                    // 確認逃生路徑走得到，否則寧可不放 (避免無腦自殺)
                    auto testSafe = FindSafeSpot(botX, botY, levelManager, bombManager, botFirepower, botX, botY);
                    if (testSafe.found) {
                        auto escapePath = FindPath(botX, botY, testSafe.x, testSafe.y, mapW, mapH, [&](int x, int y) {
                            if (!levelManager.IsWalkable(x, y) || bombManager.IsBombAt(x, y) || bombManager.HasExplosionAt(x, y)) return -1;
                            if (IsLethal(x, y, levelManager, botFirepower)) return -1;  // pretend bomb 還沒爆，不擋 path
                            if (isSpiritAt(x, y) || isTurretAt(x, y)) return -1;
                            return isBlockedByOther(x, y) ? kOtherPlayerPenalty : 1;
                            });
                        if (!escapePath.empty()) {
                            placeBomb = true;
                            registerPendingBomb(botX, botY, botFirepower);
                            // 放彈後朝逃生點走、不要傻站
                            ExecuteMove(botController, botX, botY, escapePath[0].first, escapePath[0].second, true, bot->GetPixelPos());
                            continue;
                        }
                    }
                }
            }

            botController->SetInput(false, false, false, false, placeBomb);
            continue;
        }

        botController->SetInput(false, false, false, false, false);
    }
}

bool AIManager::IsLethal(int tx, int ty, const LevelManager& levelManager, int fp, int pretendX, int pretendY) const {
    // 對地圖外或越界格子保守視為不致命 (與舊版行為一致)
    if (tx < 0 || ty < 0 || ty >= static_cast<int>(m_Danger.size()) ||
        (m_Danger.size() > 0 && tx >= static_cast<int>(m_Danger[0].size()))) {
        return false;
    }

    // 1) 真實炸彈/爆炸 — 直接查預先計算的快取
    if (m_Danger[ty][tx]) return true;

    // 2) 假想炸彈 (AI 想像在 pretendX/Y 放一顆 fp 火力的炸彈)
    if (pretendX >= 0 && pretendY >= 0) {
        if (tx == pretendX && ty == pretendY) return true;
        for (const auto& off : kCardinalOffsets) {
            for (int step = 1; step <= fp; step++) {
                int cx = pretendX + off.dx * step;
                int cy = pretendY + off.dy * step;
                if (tx == cx && ty == cy) return true;
                if (!levelManager.IsWalkable(cx, cy)) break;
            }
        }
    }
    return false;
}

AIManager::SafeSpot AIManager::FindSafeSpot(int startX, int startY, const LevelManager& levelManager, const BombManager& bombManager, int botFp, int pretendX, int pretendY) const {
    SafeSpot bestSafe = { -1, -1, 999, false };
    int mapW = levelManager.GetMapWidth();
    int mapH = levelManager.GetMapHeight();

    std::vector<std::vector<bool>> visited(mapH, std::vector<bool>(mapW, false));
    std::queue<SafeSpot> q;
    q.push({ startX, startY, 0, false });
    visited[startY][startX] = true;

    while (!q.empty()) {
        auto current = q.front();
        q.pop();

        if (!IsLethal(current.x, current.y, levelManager, botFp, pretendX, pretendY)) {
            bestSafe = { current.x, current.y, current.dist, true };
            break;
        }

        if (current.dist >= 5) continue;

        for (const auto& off : kCardinalOffsets) {
            int nx = current.x + off.dx;
            int ny = current.y + off.dy;

            if (nx >= 0 && nx < mapW && ny >= 0 && ny < mapH) {
                if (!visited[ny][nx] && levelManager.IsWalkable(nx, ny) &&
                    !bombManager.IsBombAt(nx, ny) && !bombManager.HasExplosionAt(nx, ny)) {
                    visited[ny][nx] = true;
                    q.push({ nx, ny, current.dist + 1, false });
                }
            }
        }
    }
    return bestSafe;
}

std::shared_ptr<Interactable> AIManager::FindNearestTarget(int botX, int botY, bool hasKey,
                                                            const std::vector<std::shared_ptr<Interactable>>& items,
                                                            const std::unordered_set<Interactable*>& claimed) const {
    std::shared_ptr<Interactable> nearestItem = nullptr;
    std::shared_ptr<Interactable> nearestChest = nullptr;
    std::shared_ptr<Interactable> nearestKey = nullptr;

    int itemDist = 9999, chestDist = 9999, keyDist = 9999;

    for (const auto& item : items) {
        if (claimed.count(item.get())) continue;  // 已被其他 bot 鎖定，跳過
        int dist = std::abs(item->GetGridX() - botX) + std::abs(item->GetGridY() - botY);
        if (std::dynamic_pointer_cast<PowerUp>(item)) {
            if (dist < itemDist) { itemDist = dist; nearestItem = item; }
        } else {
            auto c = std::dynamic_pointer_cast<Chest>(item);
            if (c && !c->IsOpened()) {
                if (dist < chestDist) { chestDist = dist; nearestChest = item; }
            } else if (std::dynamic_pointer_cast<Key>(item)) {
                if (dist < keyDist) { keyDist = dist; nearestKey = item; }
            }
        }
    }

    if (nearestItem) return nearestItem;
    if (hasKey && nearestChest) return nearestChest;
    if (!hasKey && nearestKey) return nearestKey;
    // 沒鑰匙的 bot 不該鎖定寶箱：自己根本打不開，還會佔住 claimedTargets 讓持鑰匙的 bot 沒得拿
    // 回 nullptr 讓 caller fallback 到追擊 defender
    return nullptr;
}

void AIManager::ExecuteMove(BotController* botController, int fromX, int fromY, int toX, int toY, bool placeBomb,
                             glm::vec2 botPixelPos) const {
    bool up = false, down = false, left = false, right = false;
    const float targetPixelX = GridCoord::ToPixelX(toX);
    const float targetPixelY = GridCoord::ToPixelY(toY);
    constexpr float kAlignTolerance = 2.0f;  // 像素級漂移容忍度

    // 主方向 (toX != fromX 或 toY != fromY) + 副方向 (校正另一軸的漂移)
    if (toX != fromX) {
        // 水平主移動：另外看 Y 軸有沒有漂走，有就補 up/down 拉回中線
        if (toX > fromX) right = true;
        else             left  = true;

        if (botPixelPos.y < targetPixelY - kAlignTolerance)       up   = true;
        else if (botPixelPos.y > targetPixelY + kAlignTolerance)  down = true;
    }
    else if (toY != fromY) {
        // 垂直主移動：另外看 X 軸有沒有漂走，有就補 left/right
        if (toY > fromY) down = true;
        else             up   = true;

        if (botPixelPos.x < targetPixelX - kAlignTolerance)       right = true;
        else if (botPixelPos.x > targetPixelX + kAlignTolerance)  left  = true;
    }

    botController->SetInput(up, down, left, right, placeBomb);
}
