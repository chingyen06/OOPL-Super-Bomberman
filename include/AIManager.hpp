#ifndef AIMANAGER_HPP
#define AIMANAGER_HPP

#include <vector>
#include <memory>
#include <utility>
#include <functional>
#include "Player.hpp"
#include "LevelManager.hpp"
#include "BombManager.hpp"
#include "InteractableManager.hpp"

class AIManager {
public:
    void Update(std::vector<std::shared_ptr<Player>>& players, const LevelManager& levelManager, const BombManager& bombManager, const InteractableManager& interactableManager);

private:
    // 泛用型 A* 演算法，將尋路規則交給 Cost Function 決定
    std::vector<std::pair<int, int>> FindPath(int startX, int startY, int targetX, int targetY, std::function<int(int, int)> costFunc);
};

#endif