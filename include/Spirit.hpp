#ifndef SPIRIT_HPP
#define SPIRIT_HPP

#include "Util/GameObject.hpp"
#include "Util/Image.hpp"
#include "glm/vec2.hpp"
#include <memory>
#include <vector>
#include "Player.hpp"

class LevelManager;
class BombManager;

class Spirit : public Util::GameObject {
public:
    enum class State { PATROL, CHASE, DEAD };

    Spirit(int spawnGridX, int spawnGridY);

    void Update(std::vector<std::shared_ptr<Player>>& players, const LevelManager& levelmanager, const BombManager& bombmanager);

    bool ShouldDelete() const { return m_ShouldDelete; }
    int GetGridX() const { return m_GridX; }
    int GetGridY() const { return m_GridY; }

private:
    State m_State = State::PATROL;
    bool m_ShouldDelete = false;
    int m_GridX, m_GridY;
    int m_SpawnX, m_SpawnY;
    glm::vec2 m_Pos;

    bool m_IsMoving = false;
    glm::vec2 m_PixelTarget;

    float m_MoveSpeed = 1.0f;
    int m_Tick = 0;
    int m_StateTimer = 0;

    std::shared_ptr<Player> m_Target = nullptr;
    std::shared_ptr<Util::Image> m_ImgIdle;

    void HandlePatrol(const LevelManager& lm, const BombManager& bm);
    void HandleChase(const LevelManager& lm, const BombManager& bm);
    void UpdatePixelMovement();
    void CheckDamage(const BombManager& bm);
    void ScanForEnemies(const std::vector<std::shared_ptr<Player>>& players);
    void MoveTowards(int targetX, int targetY, const LevelManager& lm, const BombManager& bm);
};

#endif