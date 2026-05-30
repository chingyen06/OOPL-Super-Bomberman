#ifndef GAMEWORLDCONTEXT_HPP
#define GAMEWORLDCONTEXT_HPP

#include "WorldContext.hpp"

class LevelManager;
class BombManager;
class InteractableManager;
class TurretManager;

class GameWorldContext : public IEnemyWorldContext {
public:
    GameWorldContext(const LevelManager& lm, const BombManager& bm, const InteractableManager& im, const TurretManager& tm);

    bool IsWalkable(int gridX, int gridY) const override;
    bool IsBombAt(int gridX, int gridY) const override;
    bool IsTurretAt(int gridX, int gridY) const override;
    glm::vec2 GetForceAt(int gridX, int gridY) const override;
    bool HasExplosionAt(int gridX, int gridY) const override;

private:
    const LevelManager& m_LevelManager;
    const BombManager& m_BombManager;
    const InteractableManager& m_InteractableManager;
    const TurretManager& m_TurretManager;
};

#endif
