#include "Spirit.hpp"
#include "LevelManager.hpp"
#include "BombManager.hpp"
#include "Util/Logger.hpp"
#include <cmath>
#include <cstdlib>

Spirit::Spirit(int spawnGridX, int spawnGridY)
    : m_SpawnX(spawnGridX), m_SpawnY(spawnGridY), m_GridX(spawnGridX), m_GridY(spawnGridY) {

    m_ImgIdle = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/spirit.png");
    SetDrawable(m_ImgIdle);

    SetZIndex(18);

    m_Pos.x = (m_GridX - 12) * 32.0f;
    m_Pos.y = (8 - m_GridY) * 32.0f;
    m_Transform.translation = m_Pos;
}

void Spirit::Update(std::vector<std::shared_ptr<Player>>& players, const LevelManager& lm, const BombManager& bm) {
    if (m_ShouldDelete) return;

    CheckDamage(bm);
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
            HandlePatrol(lm, bm);
            break;
        case State::CHASE:
            ScanForEnemies(players);
            HandleChase(lm, bm);
            break;
        case State::DEAD:
            break;
        }
    }

    float hover = std::sin(static_cast<float>(m_Tick) * 0.1f) * 5.0f;
    m_Transform.translation = {
        std::round(m_Pos.x),
        std::round(m_Pos.y + 10.0f + hover)
    };
}

void Spirit::CheckDamage(const BombManager& bm) {
    if (bm.HasExplosionAt(m_GridX, m_GridY)) {
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

    // 判斷是否抵達目標點
    if (dist <= m_MoveSpeed) {
        m_Pos = m_PixelTarget;
        m_IsMoving = false;
    }
    else {
        m_Pos += glm::normalize(dir) * m_MoveSpeed;
    }
}

void Spirit::ScanForEnemies(const std::vector<std::shared_ptr<Player>>& players) {
    const int alertRadius = 5; // 警戒半徑 5 格
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

    // 根據索敵結果切換狀態
    if (m_Target && m_State == State::PATROL) {
        m_State = State::CHASE;
    }
    else if (!m_Target && m_State == State::CHASE) {
        m_State = State::PATROL;
    }
}

void Spirit::HandlePatrol(const LevelManager& lm, const BombManager& bm) {
    m_StateTimer--;
    if (m_StateTimer <= 0) {
        m_StateTimer = 60; // 巡邏停頓 1 秒
        int dx = (std::rand() % 3) - 1; // -1, 0, 1
        int dy = (std::rand() % 3) - 1;

        int nextX = m_GridX + dx;
        int nextY = m_GridY + dy;

        // 限制在出生點半徑 3 格內巡邏
        if (std::abs(nextX - m_SpawnX) <= 3 && std::abs(nextY - m_SpawnY) <= 3) {
            MoveTowards(nextX, nextY, lm, bm);
        }
    }
}

void Spirit::HandleChase(const LevelManager& lm, const BombManager& bm) {
    if (!m_Target) return;

    m_StateTimer--;
    if (m_StateTimer <= 0) {
        m_StateTimer = 30; // 追擊反應頻率 (0.5 秒一次)

        int nextX = m_GridX + (m_Target->GetGridX() > m_GridX ? 1 : (m_Target->GetGridX() < m_GridX ? -1 : 0));
        int nextY = m_GridY + (m_Target->GetGridY() > m_GridY ? 1 : (m_Target->GetGridY() < m_GridY ? -1 : 0));
        MoveTowards(nextX, nextY, lm, bm);
    }
}

void Spirit::MoveTowards(int targetX, int targetY, const LevelManager& lm, const BombManager& bm) {
    if (m_IsMoving) return;

    // 檢查目標格是否可通行且無炸彈
    if (lm.IsWalkable(targetX, targetY) && !bm.IsBombAt(targetX, targetY)) {
        m_IsMoving = true;
        m_PixelTarget.x = (targetX - 12) * 32.0f;
        m_PixelTarget.y = (8 - targetY) * 32.0f;
        m_GridX = targetX;
        m_GridY = targetY;
    }
}