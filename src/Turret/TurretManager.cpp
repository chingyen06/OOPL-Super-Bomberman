#include "Turret/TurretManager.hpp"
#include "LevelManager.hpp"
#include "BombManager.hpp"

void TurretManager::AddTurret(std::shared_ptr<Turret> turret, Util::Renderer& root) {
    m_Turrets.push_back(turret);
    root.AddChild(turret);
}

void TurretManager::Update(std::vector<std::shared_ptr<Player>>& players, const LevelManager& lm, const BombManager& bm, Util::Renderer& root) {
    size_t oldProjectileCount = m_Projectiles.size();

    for (auto& turret : m_Turrets) {
        turret->Update(m_Projectiles);
    }

    for (size_t i = oldProjectileCount; i < m_Projectiles.size(); ++i) {
        root.AddChild(m_Projectiles[i]);
    }

    for (auto it = m_Projectiles.begin(); it != m_Projectiles.end();) {
        auto& bullet = *it;
        bullet->Update(players, lm, bm);

        if (bullet->IsDead()) {
            root.RemoveChild(bullet);
            it = m_Projectiles.erase(it);
        }
        else {
            ++it;
        }
    }
}

void TurretManager::Clear(Util::Renderer& root) {
    for (auto& t : m_Turrets) root.RemoveChild(t);
    for (auto& p : m_Projectiles) root.RemoveChild(p);
    m_Turrets.clear();
    m_Projectiles.clear();
}