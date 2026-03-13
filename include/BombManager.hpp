#ifndef BOMBMANAGER_HPP
#define BOMBMANAGER_HPP

#include <vector>
#include <memory>
#include "Bomb.hpp"
#include "Explosion.hpp"
#include "LevelManager.hpp"
#include "Util/Renderer.hpp"

class Player;

class BombManager {
public:
    // void PlaceBomb(int gridX, int gridY, int firepower, Util::Renderer& root);
    void PlaceBomb(std::shared_ptr<Player>& player, LevelManager& levelManager, Util::Renderer& root);
    void Update(LevelManager& levelManager, Util::Renderer& root, std::shared_ptr<Player>& player);
    void Clear(Util::Renderer& root);
    bool IsBombAt(int gridX, int gridY) const;

private:
    std::vector<std::shared_ptr<Bomb>> m_Bombs;
    std::vector<std::shared_ptr<Explosion>> m_Explosions;
};

#endif