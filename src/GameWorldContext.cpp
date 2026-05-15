#include "GameWorldContext.hpp"
#include "LevelManager.hpp"
#include "BombManager.hpp"
#include "InteractableManager.hpp"
#include "Turret/TurretManager.hpp"

GameWorldContext::GameWorldContext(const LevelManager& lm, const BombManager& bm, const InteractableManager& im, const TurretManager& tm)
    : m_LevelManager(lm), m_BombManager(bm), m_InteractableManager(im), m_TurretManager(tm) {
}

bool GameWorldContext::IsWalkable(int gridX, int gridY) const {
    return m_LevelManager.IsWalkable(gridX, gridY);
}

bool GameWorldContext::IsBombAt(int gridX, int gridY) const {
    return m_BombManager.IsBombAt(gridX, gridY);
}

glm::vec2 GameWorldContext::GetForceAt(int gridX, int gridY) const {
    return m_InteractableManager.GetForceAt(gridX, gridY);
}

bool GameWorldContext::IsTurretAt(int gridX, int gridY) const {
    return m_TurretManager.IsTurretAt(gridX, gridY);
}