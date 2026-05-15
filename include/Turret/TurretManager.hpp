#ifndef TURRETMANAGER_HPP
#define TURRETMANAGER_HPP

#include "Turret/Turret.hpp"
#include "Turret/Projectile.hpp"
#include "Util/Renderer.hpp"
#include <vector>
#include <memory>

class LevelManager;
class BombManager;
class Player;
class InteractableManager;

class TurretManager {
public:
    void AddTurret(std::shared_ptr<Turret> turret, Util::Renderer& root);
    void Update(std::vector<std::shared_ptr<Player>>& players, const LevelManager& lm, BombManager& bm, const InteractableManager& im, Util::Renderer& root);
    void Clear(Util::Renderer& root);
    bool IsTurretAt(int gridX, int gridY) const;

private:
    std::vector<std::shared_ptr<Turret>> m_Turrets;
    std::vector<std::shared_ptr<Projectile>> m_Projectiles;
};
#endif