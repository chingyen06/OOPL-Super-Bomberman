#ifndef SPIRIT_HPP
#define SPIRIT_HPP

#include "Util/GameObject.hpp"
#include "Util/Image.hpp"
#include "glm/vec2.hpp"
#include <memory>
#include <vector>
#include "GameConstants.hpp"
#include "WorldContext.hpp"

class Player;

class Spirit : public Util::GameObject {
public:
    enum class State { PATROL, CHASE, DEAD };

    Spirit(int spawnGridX, int spawnGridY);

    // 透過 IEnemyWorldContext 抽象查詢世界 (可走 / 炸彈 / 爆炸)，不再依賴具體 LevelManager / BombManager (DIP)
    void Update(std::vector<std::shared_ptr<Player>>& players, const IEnemyWorldContext& world);

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

    float m_MoveSpeed = Constants::Spirit::kMoveSpeed;
    int m_Tick = 0;
    int m_StateTimer = 0;

    std::shared_ptr<Player> m_Target = nullptr;
    std::shared_ptr<Util::Image> m_ImgIdle;

    void HandlePatrol(const IEnemyWorldContext& world);
    void HandleChase(const IEnemyWorldContext& world);
    void UpdatePixelMovement();
    void CheckDamage(const IEnemyWorldContext& world);
    void ScanForEnemies(const std::vector<std::shared_ptr<Player>>& players);
    void MoveTowards(int targetX, int targetY, const IEnemyWorldContext& world);
};

#endif