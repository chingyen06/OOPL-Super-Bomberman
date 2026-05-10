#ifndef GAMEWORLDCONTEXT_HPP
#define GAMEWORLDCONTEXT_HPP

#include "WorldContext.hpp"

// 前向宣告
class LevelManager;
class BombManager;
class InteractableManager;

class GameWorldContext : public IWorldContext {
public:
    GameWorldContext(const LevelManager& lm, const BombManager& bm, const InteractableManager& im);

    bool IsWalkable(int gridX, int gridY) const override;
    bool IsBombAt(int gridX, int gridY) const override;
    glm::vec2 GetForceAt(int gridX, int gridY) const override;

private:
    const LevelManager& m_LevelManager;
    const BombManager& m_BombManager;
    const InteractableManager& m_InteractableManager;
};

#endif
