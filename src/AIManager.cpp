#include "AIManager.hpp"
#include "GameTypes.hpp"
#include "Interactable.hpp"
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
    const InteractableManager& interactableManager) {

    RebuildDangerMap(levelManager, bombManager);

    const auto& interactables = interactableManager.GetInteractables();
    int mapW = levelManager.GetMapWidth();
    int mapH = levelManager.GetMapHeight();

    for (auto& bot : players) {
        if (bot->IsDead()) continue;
        auto botController = dynamic_cast<BotController*>(bot->GetController());
        if (!botController) continue;

        int botX = bot->GetGridX();
        int botY = bot->GetGridY();
        int botFirepower = bot->GetFirepower();

        bool inDanger = IsLethal(botX, botY, levelManager, botFirepower);
        if (inDanger) {
            auto safeSpot = FindSafeSpot(botX, botY, levelManager, bombManager, botFirepower);
            if (safeSpot.found) {
                auto path = FindPath(botX, botY, safeSpot.x, safeSpot.y, mapW, mapH, [&](int x, int y) {
                    if (!levelManager.IsWalkable(x, y) || bombManager.IsBombAt(x, y) || bombManager.HasExplosionAt(x, y)) return -1;
                    return 1;
                    });
                if (!path.empty()) ExecuteMove(botController, botX, botY, path[0].first, path[0].second, false);
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
        auto nearestTarget = FindNearestTarget(botX, botY, bot->HasKey(), interactables);

        if (nearestTarget) {
            targetX = nearestTarget->GetGridX();
            targetY = nearestTarget->GetGridY();
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
            return 1;
            });

        if (!pathSafe.empty()) {
            bool placeBomb = false;

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
                    auto testSafe = FindSafeSpot(botX, botY, levelManager, bombManager, botFirepower, botX, botY);
                    if (testSafe.found) placeBomb = true;
                }
            }

            ExecuteMove(botController, botX, botY, pathSafe[0].first, pathSafe[0].second, placeBomb);
            continue;
        }

        // 策略 4：無安全路徑 - 嘗試炸牆 (不管爆炸)
        auto pathThroughBricks = FindPath(botX, botY, targetX, targetY, mapW, mapH, [&](int x, int y) {
            if (!levelManager.IsWalkable(x, y) && !levelManager.IsBrick(x, y)) return -1;
            if (bombManager.IsBombAt(x, y) || bombManager.HasExplosionAt(x, y) || IsLethal(x, y, levelManager, botFirepower)) return -1;
            if (levelManager.IsBrick(x, y)) return 50;
            return 1;
            });

        if (!pathThroughBricks.empty()) {
            int walkToX = botX, walkToY = botY;
            for (auto& p : pathThroughBricks) {
                if (levelManager.IsBrick(p.first, p.second)) break;
                walkToX = p.first; walkToY = p.second;
            }

            if (botX == walkToX && botY == walkToY) {
                auto testSafe = FindSafeSpot(botX, botY, levelManager, bombManager, botFirepower, botX, botY);
                if (testSafe.found) botController->SetInput(false, false, false, false, true);
                else botController->SetInput(false, false, false, false, false);
            }
            else {
                auto pathToBrick = FindPath(botX, botY, walkToX, walkToY, mapW, mapH, [&](int x, int y) {
                    if (!levelManager.IsWalkable(x, y) || bombManager.IsBombAt(x, y) || bombManager.HasExplosionAt(x, y) || IsLethal(x, y, levelManager, botFirepower)) return -1;
                    return 1;
                    });
                if (!pathToBrick.empty()) ExecuteMove(botController, botX, botY, pathToBrick[0].first, pathToBrick[0].second, false);
                else botController->SetInput(false, false, false, false, false);
            }
            continue;
        }

        // 策略 5：無安全路徑 - 無視火焰與障礙 (自殺攻擊)
        auto pathIgnoreFire = FindPath(botX, botY, targetX, targetY, mapW, mapH, [&](int x, int y) {
            if (!levelManager.IsWalkable(x, y)) return -1;
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
                    auto testSafe = FindSafeSpot(botX, botY, levelManager, bombManager, botFirepower, botX, botY);
                    if (testSafe.found) placeBomb = true;
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

std::shared_ptr<Interactable> AIManager::FindNearestTarget(int botX, int botY, bool hasKey, const std::vector<std::shared_ptr<Interactable>>& items) const {
    std::shared_ptr<Interactable> nearestItem = nullptr;
    std::shared_ptr<Interactable> nearestChest = nullptr;
    std::shared_ptr<Interactable> nearestKey = nullptr;

    int itemDist = 9999, chestDist = 9999, keyDist = 9999;

    for (const auto& item : items) {
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
    if (!hasKey && nearestChest) return nearestChest;

    return nullptr;
}

void AIManager::ExecuteMove(BotController* botController, int fromX, int fromY, int toX, int toY, bool placeBomb) const {
    bool up = false, down = false, left = false, right = false;
    if (toX > fromX) right = true;
    else if (toX < fromX) left = true;
    else if (toY > fromY) down = true;
    else if (toY < fromY) up = true;
    botController->SetInput(up, down, left, right, placeBomb);
}
