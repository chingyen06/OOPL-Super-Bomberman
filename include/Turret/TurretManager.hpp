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

class TurretManager {
public:
    void AddTurret(std::shared_ptr<Turret> turret, Util::Renderer& root);
    void Update(std::vector<std::shared_ptr<Player>>& players, const LevelManager& lm, const BombManager& bm, Util::Renderer& root);
    void Clear(Util::Renderer& root);

private:
    std::vector<std::shared_ptr<Turret>> m_Turrets;
    std::vector<std::shared_ptr<Projectile>> m_Projectiles;
};
#endif