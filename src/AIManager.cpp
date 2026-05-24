#include "AIManager.hpp"
#include "GameConstants.hpp"
#include "GameTypes.hpp"
#include "GridCoord.hpp"
#include "Interactable.hpp"
#include "Spirit.hpp"
#include "Turret/TurretManager.hpp"
#include "Controller/IProgrammableController.hpp"
#include <queue>
#include <cmath>
#include <algorithm>
#include <functional>
#include <map>

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

void AIManager::Update(std::vector<std::shared_ptr<Player>>& players,
    const LevelManager& levelManager,
    const BombManager& bombManager,
    const InteractableManager& interactableManager,
    const std::vector<std::shared_ptr<Spirit>>& spirits,
    const TurretManager& turretManager) {

    m_DangerMap.Rebuild(levelManager, bombManager);

    const auto& interactables = interactableManager.GetInteractables();
    int mapW = levelManager.GetMapWidth();
    int mapH = levelManager.GetMapHeight();

    // 跨 bot 共享的目標鎖定表：本幀內某物件被某 bot 鎖定後，其他 bot 不再追同一物
    std::unordered_set<Interactable*> claimedTargets;

    // pending 炸彈的危險範圍委由 DangerMap 處理 — 防止多隻 bot 各自以為安全、放完發現連鎖把全員炸死
    auto registerPendingBomb = [&](int bx, int by, int fp) {
        m_DangerMap.RegisterPendingBomb(bx, by, fp, levelManager);
    };

    for (auto& bot : players) {
        if (bot->IsDead()) continue;
        // Cross-cast 到 IProgrammableController：HumanController 沒有實作此介面所以會回 nullptr，
        // 自動被略過 — 不再依賴具體 BotController 類別。
        auto botController = dynamic_cast<IProgrammableController*>(bot->GetController());
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
        const bool inDanger = m_DangerMap.IsLethal(botX, botY, levelManager, botFirepower);
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
            IProgrammableController* ctrl;
            int frames;
            ~ResetGuard() { ctrl->ResetCooldown(frames); }
        };
        const int nextCooldown = needsImmediate
            ? 0
            : Constants::Bot::kReactionFrames + (bot->GetPlayerID() % 3);
        ResetGuard guard{ botController, nextCooldown };

        if (inDanger) {
            auto safeSpot = m_DangerMap.FindSafeSpot(botX, botY, levelManager, bombManager, botFirepower);
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
            if (m_DangerMap.IsLethal(x, y, levelManager, botFirepower)) return -1;
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
                auto testSafe = m_DangerMap.FindSafeSpot(botX, botY, levelManager, bombManager, botFirepower, botX, botY);
                if (testSafe.found) {
                    // 注意：escape path 只避開「已存在」的炸彈/爆炸，不避開 pretend bomb 的火力範圍。
                    // 因為剛放的炸彈有 3 秒引信，bot 有充足時間從尚未爆炸的格子穿出去 (testSafe.dist ≤ 5)。
                    auto escapePath = FindPath(botX, botY, testSafe.x, testSafe.y, mapW, mapH, [&](int x, int y) {
                        if (!levelManager.IsWalkable(x, y) || bombManager.IsBombAt(x, y) || bombManager.HasExplosionAt(x, y)) return -1;
                        if (m_DangerMap.IsLethal(x, y, levelManager, botFirepower)) return -1;  // 真實炸彈/爆炸才擋
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
            if (bombManager.IsBombAt(x, y) || bombManager.HasExplosionAt(x, y) || m_DangerMap.IsLethal(x, y, levelManager, botFirepower)) return -1;
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
                auto testSafe = m_DangerMap.FindSafeSpot(botX, botY, levelManager, bombManager, botFirepower, botX, botY);
                bool canBomb = false;
                int moveToX = botX, moveToY = botY;
                if (testSafe.found) {
                    auto escapePath = FindPath(botX, botY, testSafe.x, testSafe.y, mapW, mapH, [&](int x, int y) {
                        if (!levelManager.IsWalkable(x, y) || bombManager.IsBombAt(x, y) || bombManager.HasExplosionAt(x, y)) return -1;
                        if (m_DangerMap.IsLethal(x, y, levelManager, botFirepower)) return -1;  // pretend bomb 還沒爆，不擋 path
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
                    if (!levelManager.IsWalkable(x, y) || bombManager.IsBombAt(x, y) || bombManager.HasExplosionAt(x, y) || m_DangerMap.IsLethal(x, y, levelManager, botFirepower)) return -1;
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
                    auto testSafe = m_DangerMap.FindSafeSpot(botX, botY, levelManager, bombManager, botFirepower, botX, botY);
                    if (testSafe.found) {
                        auto escapePath = FindPath(botX, botY, testSafe.x, testSafe.y, mapW, mapH, [&](int x, int y) {
                            if (!levelManager.IsWalkable(x, y) || bombManager.IsBombAt(x, y) || bombManager.HasExplosionAt(x, y)) return -1;
                            if (m_DangerMap.IsLethal(x, y, levelManager, botFirepower)) return -1;  // pretend bomb 還沒爆，不擋 path
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

std::shared_ptr<Interactable> AIManager::FindNearestTarget(int botX, int botY, bool hasKey,
                                                            const std::vector<std::shared_ptr<Interactable>>& items,
                                                            const std::unordered_set<Interactable*>& claimed) const {
    // 優先序資料化：不再 dynamic_pointer_cast<PowerUp/Chest/Key>，改詢問 Interactable
    // 的 GetAttackerTargetPriority。每個 priority 層內取距離最近者，跨層則依 std::map
    // 排序 (priority 小 = 優先) 由低到高挑第一個非空層。
    //
    // 新增一種 bot 目標型別 = 新類別 override GetAttackerTargetPriority — 不必動 AIManager。
    std::map<int, std::pair<std::shared_ptr<Interactable>, int>> bestByPriority;

    for (const auto& item : items) {
        if (claimed.count(item.get())) continue;  // 已被其他 bot 鎖定
        const int prio = item->GetAttackerTargetPriority(hasKey);
        if (prio <= 0) continue;  // 不可選為目標 (e.g. 沒鑰匙的 bot 看到 Chest)

        const int dist = std::abs(item->GetGridX() - botX) + std::abs(item->GetGridY() - botY);
        auto it = bestByPriority.find(prio);
        if (it == bestByPriority.end() || dist < it->second.second) {
            bestByPriority[prio] = { item, dist };
        }
    }

    if (bestByPriority.empty()) return nullptr;
    return bestByPriority.begin()->second.first;  // 最低 priority 編號 = 最優先
}

void AIManager::ExecuteMove(IProgrammableController* botController, int fromX, int fromY, int toX, int toY, bool placeBomb,
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
