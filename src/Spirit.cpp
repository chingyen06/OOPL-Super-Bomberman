#include "Spirit.hpp"
#include "Player.hpp"
#include "GameConstants.hpp"
#include "GameTypes.hpp"
#include "GridCoord.hpp"
#include "Util/Logger.hpp"
#include <cmath>
#include <random>

Spirit::Spirit(int spawnGridX, int spawnGridY)
    : m_SpawnX(spawnGridX), m_SpawnY(spawnGridY), m_GridX(spawnGridX), m_GridY(spawnGridY) {

    m_ImgIdle = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/spirit.png");
    SetDrawable(m_ImgIdle);

    SetZIndex(18);

    m_Pos = GridCoord::ToPixel(m_GridX, m_GridY);
    m_Transform.translation = m_Pos;
}

void Spirit::Update(std::vector<std::shared_ptr<Player>>& players, const IEnemyWorldContext& world) {
    if (m_ShouldDelete) return;

    CheckDamage(world);
    if (m_ShouldDelete) return;

    UpdatePixelMovement();
    m_Tick++;

    for (auto& p : players) {
        if (p->GetTeam() == Team::ATTACKER && !p->IsDead()) {
            if (p->GetGridX() == m_GridX && p->GetGridY() == m_GridY) {
                p->Kill();
                LOG_INFO("Attacker was caught and killed by Spirit!");
            }
        }
    }

    if (!m_IsMoving) {
        switch (m_State) {
        case State::PATROL:
            ScanForEnemies(players);
            HandlePatrol(world);
            break;
        case State::CHASE:
            ScanForEnemies(players);
            HandleChase(world);
            break;
        case State::DEAD:
            break;
        }
    }

    float hover = std::sin(static_cast<float>(m_Tick) * Constants::Spirit::kHoverSpeed) * Constants::Spirit::kHoverAmplitude;
    m_Transform.translation = {
        std::round(m_Pos.x),
        std::round(m_Pos.y + Constants::Spirit::kVisualYOffset + hover)
    };
}

void Spirit::CheckDamage(const IEnemyWorldContext& world) {
    if (world.HasExplosionAt(m_GridX, m_GridY)) {
        m_ShouldDelete = true;
        m_State = State::DEAD;
        SetVisible(false);
        LOG_INFO("Spirit Ellon was destroyed by explosion!");
    }
}

void Spirit::UpdatePixelMovement() {
    if (!m_IsMoving) return;

    glm::vec2 dir = m_PixelTarget - m_Pos;
    float dist = glm::length(dir);

    // Check if we have reached the target tile
    if (dist <= m_MoveSpeed) {
        m_Pos = m_PixelTarget;
        m_IsMoving = false;
    }
    else {
        m_Pos += glm::normalize(dir) * m_MoveSpeed;
    }
}

void Spirit::ScanForEnemies(const std::vector<std::shared_ptr<Player>>& players) {
    const int alertRadius = Constants::Spirit::kAlertRadius;
    int closestDist = 999;
    m_Target = nullptr;

    for (const auto& p : players) {
        if (p->GetTeam() == Team::ATTACKER && !p->IsDead()) {
            int dist = std::abs(p->GetGridX() - m_GridX) + std::abs(p->GetGridY() - m_GridY);
            if (dist <= alertRadius && dist < closestDist) {
                closestDist = dist;
                m_Target = p;
            }
        }
    }

    // Switch state based on whether an enemy is in range
    if (m_Target && m_State == State::PATROL) {
        m_State = State::CHASE;
    }
    else if (!m_Target && m_State == State::CHASE) {
        m_State = State::PATROL;
    }
}

void Spirit::HandlePatrol(const IEnemyWorldContext& world) {
    m_StateTimer--;
    if (m_StateTimer <= 0) {
        m_StateTimer = Constants::Spirit::kPatrolInterval;
        // 只走 4 個正方向 (避免斜向穿牆角)
        // 與專案其他處 (GameSession / InteractableManager) 一致使用 mt19937
        static thread_local std::mt19937 rng{ std::random_device{}() };
        std::uniform_int_distribution<int> pickDir(0, 3);
        const int dir = pickDir(rng);
        const auto off = kCardinalOffsets[dir];
        int nextX = m_GridX + off.dx;
        int nextY = m_GridY + off.dy;

        // Stay within kPatrolRange tiles of the spawn point
        if (std::abs(nextX - m_SpawnX) <= Constants::Spirit::kPatrolRange &&
            std::abs(nextY - m_SpawnY) <= Constants::Spirit::kPatrolRange) {
            MoveTowards(nextX, nextY, world);
        }
    }
}

void Spirit::HandleChase(const IEnemyWorldContext& world) {
    if (!m_Target) return;

    m_StateTimer--;
    if (m_StateTimer <= 0) {
        m_StateTimer = Constants::Spirit::kChaseInterval;

        // 4 方向追擊：選與目標差距較大的那一軸先走，避免斜向穿牆角
        const int dxToTarget = m_Target->GetGridX() - m_GridX;
        const int dyToTarget = m_Target->GetGridY() - m_GridY;
        int nextX = m_GridX;
        int nextY = m_GridY;
        if (std::abs(dxToTarget) >= std::abs(dyToTarget) && dxToTarget != 0) {
            nextX += (dxToTarget > 0 ? 1 : -1);
        } else if (dyToTarget != 0) {
            nextY += (dyToTarget > 0 ? 1 : -1);
        }
        MoveTowards(nextX, nextY, world);
    }
}

void Spirit::MoveTowards(int targetX, int targetY, const IEnemyWorldContext& world) {
    if (m_IsMoving) return;

    // Verify the target tile is walkable and bomb-free
    if (world.IsWalkable(targetX, targetY) && !world.IsBombAt(targetX, targetY)) {
        m_IsMoving = true;
        m_PixelTarget = GridCoord::ToPixel(targetX, targetY);
        m_GridX = targetX;
        m_GridY = targetY;
    }
}
