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
    std::vector<std::pair<int, int>> FindPath(int startX, int startY, int targetX, int targetY, int mapW, int mapH, std::function<int(int, int)> costFunc);

private:
    bool IsLethal(int tx, int ty, const LevelManager& levelManager, const BombManager& bombManager, int fp, int pretendX = -1, int pretendY = -1) const;
    
    struct SafeSpot { int x, y, dist; bool found; };
    SafeSpot FindSafeSpot(int startX, int startY, const LevelManager& levelManager, const BombManager& bombManager, int botFp, int pretendX = -1, int pretendY = -1) const;

    std::shared_ptr<Interactable> FindNearestTarget(int botX, int botY, bool hasKey, const std::vector<std::shared_ptr<Interactable>>& items) const;

    void ExecuteMove(BotController* botController, std::shared_ptr<Player>& bot, int fromX, int fromY, int toX, int toY, bool placeBomb) const;
};

#endif