#include "AIManager.hpp"
#include "Interactable.hpp"
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

std::vector<std::pair<int, int>> AIManager::FindPath(int startX, int startY, int targetX, int targetY, std::function<int(int, int)> costFunc) {
    std::vector<std::pair<int, int>> path;
    if (startX == targetX && startY == targetY) return path;

    bool closedList[17][25] = { false };
    std::priority_queue<std::shared_ptr<AStarNode>, std::vector<std::shared_ptr<AStarNode>>, CompareNode> openList;

    int startH = std::abs(startX - targetX) + std::abs(startY - targetY);
    openList.push(std::make_shared<AStarNode>(AStarNode{ startX, startY, 0, startH, nullptr }));

    int dx[] = { 0, 0, -1, 1 };
    int dy[] = { -1, 1, 0, 0 };

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

        for (int i = 0; i < 4; ++i) {
            int nx = current->x + dx[i];
            int ny = current->y + dy[i];

            if (nx >= 0 && nx < 25 && ny >= 0 && ny < 17) {
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
    const InteractableManager& interactableManager) {

    const auto& interactables = interactableManager.GetInteractables();

    // 致命區域判定 (十字火海預測 + 真實火焰)
    auto isLethal = [&](int tx, int ty, int pretendX = -1, int pretendY = -1) {
        if (bombManager.HasExplosionAt(tx, ty)) return true;

        auto checkCross = [&](int bx, int by) {
            if (tx == bx && ty == by) return true;
            int dx[] = { 0, 0, -1, 1 };
            int dy[] = { -1, 1, 0, 0 };
            for (int dir = 0; dir < 4; dir++) {
                for (int step = 1; step <= 2; step++) { // 預設火力 2
                    int cx = bx + dx[dir] * step;
                    int cy = by + dy[dir] * step;
                    if (cx == tx && cy == ty) return true;
                    if (!levelManager.IsWalkable(cx, cy)) break;
                }
            }
            return false;
            };

        for (int y = 0; y < 17; y++) {
            for (int x = 0; x < 25; x++) {
                if (bombManager.IsBombAt(x, y)) {
                    if (checkCross(x, y)) return true;
                }
            }
        }
        if (pretendX != -1 && pretendY != -1) {
            if (checkCross(pretendX, pretendY)) return true;
        }
        return false;
        };

    // BFS 求生預測系統
    struct SafeSpot { int x, y, dist; bool found; };
    auto findSafeSpotBFS = [&](int startX, int startY, int pretendBombX = -1, int pretendBombY = -1) -> SafeSpot {
        if (!isLethal(startX, startY, pretendBombX, pretendBombY)) return { startX, startY, 0, true };

        bool visited[17][25] = { false };
        std::queue<std::pair<int, int>> q;
        std::queue<int> dist_q;

        q.push({ startX, startY }); dist_q.push(0);
        visited[startY][startX] = true;
        int dx[] = { 0, 0, -1, 1 }; int dy[] = { -1, 1, 0, 0 };

        while (!q.empty()) {
            auto [cx, cy] = q.front(); q.pop();
            int dist = dist_q.front(); dist_q.pop();

            if (!isLethal(cx, cy, pretendBombX, pretendBombY)) return { cx, cy, dist, true };

            for (int i = 0; i < 4; i++) {
                int nx = cx + dx[i]; int ny = cy + dy[i];
                if (nx >= 0 && nx < 25 && ny >= 0 && ny < 17) {
                    bool isBomb = bombManager.IsBombAt(nx, ny) || bombManager.HasExplosionAt(nx, ny);
                    if (nx == pretendBombX && ny == pretendBombY) isBomb = true;
                    if (!visited[ny][nx] && levelManager.IsWalkable(nx, ny) && (!isBomb || (nx == startX && ny == startY))) {
                        visited[ny][nx] = true;
                        q.push({ nx, ny }); dist_q.push(dist + 1);
                    }
                }
            }
        }
        return { -1, -1, 999, false };
        };

    // 移動執行器：嚴格單軸驅動 + 像素對齊防卡死
    auto ExecuteMove = [&](std::shared_ptr<Player>& bot, int bX, int bY, int nX, int nY, bool placeBomb) {
        bool up = false, down = false, left = false, right = false;
        float targetPixelX = (nX - 12) * 32.0f; float targetPixelY = (8 - nY) * 32.0f;
        glm::vec2 pos = bot->GetPixelPos();

        if (nX != bX) {
            if (pos.y < targetPixelY - 2.0f) up = true;
            else if (pos.y > targetPixelY + 2.0f) down = true;
            else { if (nX > bX) right = true; if (nX < bX) left = true; }
        }
        else if (nY != bY) {
            if (pos.x < targetPixelX - 2.0f) right = true;
            else if (pos.x > targetPixelX + 2.0f) left = true;
            else { if (nY > bY) down = true; if (nY < bY) up = true; }
        }
        bot->SetBotInput(up, down, left, right, placeBomb);
        };

    for (auto& bot : players) {
        if (!bot->IsBot() || bot->IsDead()) continue;

        int botX = bot->GetGridX();
        int botY = bot->GetGridY();

        // 節點 1：我有危險嗎？ (Survival Override)
        bool inDanger = isLethal(botX, botY);
        if (inDanger) {
            auto safeSpot = findSafeSpotBFS(botX, botY);
            if (safeSpot.found) {
                auto path = FindPath(botX, botY, safeSpot.x, safeSpot.y, [&](int x, int y) {
                    if (!levelManager.IsWalkable(x, y) || bombManager.IsBombAt(x, y) || bombManager.HasExplosionAt(x, y)) return -1;
                    return 1;
                    });
                if (!path.empty()) ExecuteMove(bot, botX, botY, path[0].first, path[0].second, false);
            }
            continue;
        }

        // 節點 2：確定最終目標 (Target Selection)
        int targetX = -1, targetY = -1;
        auto findNearest = [&](auto filter) -> std::shared_ptr<Interactable> {
            std::shared_ptr<Interactable> nearest = nullptr; int minDist = 9999;
            for (const auto& item : interactables) {
                if (filter(item)) {
                    int dist = std::abs(item->GetGridX() - botX) + std::abs(item->GetGridY() - botY);
                    if (dist < minDist) { minDist = dist; nearest = item; }
                }
            }
            return nearest;
            };

        auto nearestItem = findNearest([](const std::shared_ptr<Interactable>& item) { return std::dynamic_pointer_cast<PowerUp>(item) != nullptr; });
        if (nearestItem) { targetX = nearestItem->GetGridX(); targetY = nearestItem->GetGridY(); }
        else if (bot->HasKey()) {
            auto chest = findNearest([](const std::shared_ptr<Interactable>& item) { auto c = std::dynamic_pointer_cast<Chest>(item); return c != nullptr && !c->IsOpened(); });
            if (chest) { targetX = chest->GetGridX(); targetY = chest->GetGridY(); }
        }
        else {
            auto key = findNearest([](const std::shared_ptr<Interactable>& item) { return std::dynamic_pointer_cast<Key>(item) != nullptr; });
            if (key) { targetX = key->GetGridX(); targetY = key->GetGridY(); }
        }

        if (targetX == -1) {
            for (const auto& p : players) { if (p->GetTeam() == Team::DEFENDER && !p->IsDead()) { targetX = p->GetGridX(); targetY = p->GetGridY(); break; } }
        }

        // 節點 3：嘗試安全抵達目標 (Pathfinding: Safe)
        auto pathSafe = FindPath(botX, botY, targetX, targetY, [&](int x, int y) {
            if (!levelManager.IsWalkable(x, y) || bombManager.IsBombAt(x, y) || bombManager.HasExplosionAt(x, y)) return -1;
            if (isLethal(x, y)) return -1;
            return 1;
            });

        if (!pathSafe.empty()) {
            ExecuteMove(bot, botX, botY, pathSafe[0].first, pathSafe[0].second, false);
            continue;
        }

        // 節點 4：路徑受阻分析 - 火焰等待 (Patience Veto)
        auto pathIgnoreFire = FindPath(botX, botY, targetX, targetY, [&](int x, int y) {
            if (!levelManager.IsWalkable(x, y)) return -1;
            return 1;
            });

        if (!pathIgnoreFire.empty()) {
            bot->SetBotInput(false, false, false, false, false);
            continue;
        }

        // 節點 5：路徑受阻分析 - 磚塊破壞 (Offensive Bombing)
        auto pathThroughBricks = FindPath(botX, botY, targetX, targetY, [&](int x, int y) {
            if (!levelManager.IsWalkable(x, y) && !levelManager.IsBrick(x, y)) return -1;
            if (bombManager.IsBombAt(x, y) || bombManager.HasExplosionAt(x, y) || isLethal(x, y)) return -1;
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
                // 防自殺預測
                auto testSafe = findSafeSpotBFS(botX, botY, botX, botY);
                if (testSafe.found) bot->SetBotInput(false, false, false, false, true);
                else bot->SetBotInput(false, false, false, false, false);
            }
            else {
                auto pathToBrick = FindPath(botX, botY, walkToX, walkToY, [&](int x, int y) {
                    if (!levelManager.IsWalkable(x, y) || bombManager.IsBombAt(x, y) || bombManager.HasExplosionAt(x, y) || isLethal(x, y)) return -1;
                    return 1;
                    });
                if (!pathToBrick.empty()) ExecuteMove(bot, botX, botY, pathToBrick[0].first, pathToBrick[0].second, false);
                else bot->SetBotInput(false, false, false, false, false);
            }
        }
        else {
            bot->SetBotInput(false, false, false, false, false);
        }
    }
}