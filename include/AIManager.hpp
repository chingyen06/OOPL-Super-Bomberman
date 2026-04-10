#ifndef AIMANAGER_HPP
#define AIMANAGER_HPP

#include <vector>
#include <memory>
#include <utility>
#include "Player.hpp"
#include "LevelManager.hpp"
#include "BombManager.hpp"
#include "InteractableManager.hpp"

class AIManager {
public:
    void Update(std::vector<std::shared_ptr<Player>>& players, const LevelManager& levelManager, const BombManager& bombManager, const InteractableManager& interactableManager);

private:
    // A* ºtºâªk
    std::vector<std::pair<int, int>> FindPath(int startX, int startY, int targetX, int targetY,
        const LevelManager& levelManager, const BombManager& bombManager);
};

#endif