#ifndef BOMBMANAGER_HPP
#define BOMBMANAGER_HPP

#include <vector>
#include <memory>
#include "Bomb.hpp"
#include "Explosion.hpp"
#include "LevelManager.hpp"
#include "InteractableManager.hpp"
#include "Util/Renderer.hpp"

class Player;
class TurretManager;

class BombManager {
public:
    void PlaceBomb(Player& player, LevelManager& levelManager, InteractableManager& interactableManager, Util::Renderer& root, const std::vector<std::shared_ptr<Player>>& players);
    // turretManager：火焰不可蔓延到砲台 (砲台擋火、且自身不被火焰摧毀)
    void Update(LevelManager& levelManager, InteractableManager& interactableManager, const TurretManager& turretManager, Util::Renderer& root, std::vector<std::shared_ptr<Player>>& players);
    void Clear(Util::Renderer& root);
    bool IsBombAt(int gridX, int gridY) const;
    bool HasExplosionAt(int gridX, int gridY) const;
    int GetFirepowerAt(int gridX, int gridY) const;
    void SpawnBomb(int gridX, int gridY, int firepower, int ownerID, Util::Renderer& root);

private:
    std::vector<std::shared_ptr<Bomb>> m_Bombs;
    std::vector<std::shared_ptr<Explosion>> m_Explosions;
};

#endif