#ifndef PROJECTILE_HPP
#define PROJECTILE_HPP

#include "Util/GameObject.hpp"
#include "Util/Image.hpp"
#include "Player.hpp"
#include <memory>
#include <vector>
#include "glm/vec2.hpp"

class LevelManager;
class BombManager;

class Projectile : public Util::GameObject {
public:
    Projectile(int startGridX, int startGridY, Player::Direction dir);

    void Update(std::vector<std::shared_ptr<Player>>& players, const LevelManager& lm, const BombManager& bm);

    bool IsDead() const { return m_IsDead; }

private:
    bool m_IsDead = false;
    glm::vec2 m_Pos;
    Player::Direction m_Dir;
    float m_Speed = 6.0f;

    void CheckCollision(std::vector<std::shared_ptr<Player>>& players,
        const LevelManager& lm,
        const BombManager& bm);
};

#endif