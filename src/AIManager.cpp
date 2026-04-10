#include "AIManager.hpp"
#include "Interactable.hpp"
#include <queue>
#include <cmath>
#include <algorithm>

// 定義 A* 演算法中的節點
struct AStarNode {
    int x, y;
    int g, h;
    std::shared_ptr<AStarNode> parent;

    int f() const { return g + h; }
};

// Min-Heap: f 值越小越優先
struct CompareNode {
    bool operator()(const std::shared_ptr<AStarNode>& a, const std::shared_ptr<AStarNode>& b) {
        return a->f() > b->f();
    }
};

std::vector<std::pair<int, int>> AIManager::FindPath(int startX, int startY, int targetX, int targetY,
    const LevelManager& levelManager, const BombManager& bombManager) {
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
                bool isWalkable = levelManager.IsWalkable(nx, ny);
                bool isBrick = levelManager.IsBrick(nx, ny);
                bool isBomb = bombManager.IsBombAt(nx, ny);

                // 允許把磚塊當作路徑，不踩上已經放好的炸彈
                if (!closedList[ny][nx] && (isWalkable || isBrick) && !isBomb) {
                    // 磚塊成本極高 (50)，草地成本低 (1)，確保 AI 優先走空地
                    int penalty = isBrick ? 50 : 1; 

                    // 如果這格在炸彈的十字線上，加上成本
                    for (int d = -2; d <= 2; d++) {
                        if ((nx + d >= 0 && nx + d < 25 && bombManager.IsBombAt(nx + d, ny)) ||
                            (ny + d >= 0 && ny + d < 17 && bombManager.IsBombAt(nx, ny + d))) {
                            penalty += 100;
                        }
                    }

                    int g = current->g + penalty;
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

    const auto& interactables = interactableManager.GetInteractables(); // 找物件

    for (auto& bot : players) {
        if (!bot->IsBot() || bot->IsDead()) continue;

        int botX = bot->GetGridX();
        int botY = bot->GetGridY();
        int targetX = -1, targetY = -1;

        // 如果身處炸彈十字範圍內，尋找最近的安全點
        bool inDanger = false;
        for (int d = -2; d <= 2; d++) {
            if ((botX + d >= 0 && botX + d < 25 && bombManager.IsBombAt(botX + d, botY)) ||
                (botY + d >= 0 && botY + d < 17 && bombManager.IsBombAt(botX, botY + d))) {
                inDanger = true;
                break;
            }
        }

        if (inDanger) {
            int minDist = 9999;
            for (int y = 0; y < 17; y++) {
                for (int x = 0; x < 25; x++) {
                    if (levelManager.IsWalkable(x, y) && !bombManager.IsBombAt(x, y)) {
                        bool safe = true;
                        for (int d = -2; d <= 2; d++) {
                            if ((x + d >= 0 && x + d < 25 && bombManager.IsBombAt(x + d, y)) ||
                                (y + d >= 0 && y + d < 17 && bombManager.IsBombAt(x, y + d))) {
                                safe = false; break;
                            }
                        }
                        if (safe) {
                            int dist = std::abs(x - botX) + std::abs(y - botY);
                            if (dist < minDist) {
                                minDist = dist;
                                targetX = x; targetY = y;
                            }
                        }
                    }
                }
            }
        } 
        else {
            // 找道具 -> 找鑰匙 -> 找寶箱 -> 追殺
            auto findNearest = [&](auto filter) -> std::shared_ptr<Interactable> {
                std::shared_ptr<Interactable> nearest = nullptr;
                int minDist = 9999;
                for (const auto& item : interactables) {
                    if (filter(item)) {
                        int dist = std::abs(item->GetGridX() - botX) + std::abs(item->GetGridY() - botY);
                        if (dist < minDist) {
                            minDist = dist;
                            nearest = item;
                        }
                    }
                }
                return nearest;
            };

            // 優先找近距離的加速鞋
            auto nearestItem = findNearest([](const std::shared_ptr<Interactable>& item) {
                return std::dynamic_pointer_cast<PowerUp>(item) != nullptr;
            });
            if (nearestItem && (std::abs(nearestItem->GetGridX() - botX) + std::abs(nearestItem->GetGridY() - botY)) <= 5) {
                targetX = nearestItem->GetGridX();
                targetY = nearestItem->GetGridY();
            }
            else {
                if (bot->HasKey()) { // 有鑰匙，找未開啟的寶箱
                    auto nearestChest = findNearest([](const std::shared_ptr<Interactable>& item) {
                        auto chest = std::dynamic_pointer_cast<Chest>(item);
                        return chest != nullptr && !chest->IsOpened();
                    });
                    if (nearestChest) {
                        targetX = nearestChest->GetGridX();
                        targetY = nearestChest->GetGridY();
                    }
                } else { // 沒鑰匙，找鑰匙
                    auto nearestKey = findNearest([](const std::shared_ptr<Interactable>& item) {
                        return std::dynamic_pointer_cast<Key>(item) != nullptr;
                    });
                    if (nearestKey) {
                        targetX = nearestKey->GetGridX();
                        targetY = nearestKey->GetGridY();
                    }
                }
            }

            // 沒事做就追殺防守方
            if (targetX == -1 || targetY == -1) {
                for (const auto& p : players) {
                    if (p->GetTeam() == Team::DEFENDER && !p->IsDead()) {
                        targetX = p->GetGridX();
                        targetY = p->GetGridY();
                        break;
                    }
                }
            }
        }

        // 如果真的什麼都找不到，原地發呆
        if (targetX == -1 || targetY == -1) {
            bot->SetBotInput(false, false, false, false, false);
            continue;
        }

        // 執行 A* 尋路
        auto path = FindPath(botX, botY, targetX, targetY, levelManager, bombManager);

        bool up = false, down = false, left = false, right = false, placeBomb = false;

        if (!path.empty()) {
            int nextX = path[0].first;
            int nextY = path[0].second;

            // 如果下一步是磚塊，代表被擋住了，放炸彈
            if (levelManager.IsBrick(nextX, nextY)) {
                placeBomb = true;
            } else {
                // 正常往下一格移動
                /*float centerX = (nextX - 12) * 32.0f;
                float centerY = (8 - nextY) * 32.0f;
                glm::vec2 pos = bot->GetPixelPos();

                if (pos.x < centerX - 8.0f) right = true;
                else if (pos.x > centerX + 8.0f) left = true;

                if (pos.y < centerY - 8.0f) up = true;
                else if (pos.y > centerY + 8.0f) down = true;*/
                if (nextX > botX) right = true;
                else if (nextX < botX) left = true;
                else if (nextY > botY) down = true;
                else if (nextY < botY) up = true;
            }
        }

        bot->SetBotInput(up, down, left, right, placeBomb);
    }
}