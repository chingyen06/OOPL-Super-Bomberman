#include "Bot/Pathfinder.hpp"

#include <algorithm>
#include <cmath>
#include <queue>

#include "GameTypes.hpp"  // kCardinalOffsets

std::vector<std::pair<int, int>> Pathfinder::FindPath(int startX, int startY, int targetX, int targetY,
                                                      int mapW, int mapH,
                                                      const std::function<int(int, int)>& costFunc) const {
    std::vector<std::pair<int, int>> path;
    if (startX == targetX && startY == targetY) return path;

    std::vector<std::vector<bool>> closedList(mapH, std::vector<bool>(mapW, false));
    std::priority_queue<std::shared_ptr<AStarNode>, std::vector<std::shared_ptr<AStarNode>>, CompareNode> openList;

    int startH = std::abs(startX - targetX) + std::abs(startY - targetY);
    openList.push(std::make_shared<AStarNode>(startX, startY, 0, startH, nullptr));

    while (!openList.empty()) {
        auto current = openList.top();
        openList.pop();

        if (closedList[current->Y()][current->X()]) continue;
        closedList[current->Y()][current->X()] = true;

        if (current->X() == targetX && current->Y() == targetY) {
            auto node = current;
            while (node->Parent() != nullptr) {
                path.push_back({ node->X(), node->Y() });
                node = node->Parent();
            }
            std::reverse(path.begin(), path.end());
            return path;
        }

        for (const auto& off : kCardinalOffsets) {
            int nx = current->X() + off.dx;
            int ny = current->Y() + off.dy;
            if (nx >= 0 && nx < mapW && ny >= 0 && ny < mapH) {
                int cost = costFunc(nx, ny);
                if (cost >= 0 && !closedList[ny][nx]) {
                    int g = current->G() + cost;
                    int h = std::abs(nx - targetX) + std::abs(ny - targetY);
                    openList.push(std::make_shared<AStarNode>(nx, ny, g, h, current));
                }
            }
        }
    }
    return path;
}
