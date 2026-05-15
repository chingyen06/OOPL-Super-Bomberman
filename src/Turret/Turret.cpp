#include "Turret/Turret.hpp"
#include "LevelManager.hpp"
#include "BombManager.hpp"
#include "InteractableManager.hpp"
#include <cmath>

Turret::Turret(int gridX, int gridY, Player::Direction dir)
    : m_GridX(gridX), m_GridY(gridY), m_Dir(dir), m_Timer(60) {
    m_ImgActive = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/turret_down.png");
    m_ImgIdle = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/turret_down.png");

    SetDrawable(m_ImgIdle);
    SetZIndex(18);
    m_Transform.translation = { (gridX - 12) * 32.0f, (8 - gridY) * 32.0f };
    UpdateRotationVisual();
}

void Turret::UpdateRotationVisual() {
    if (m_Dir == Player::Direction::DOWN) m_Transform.rotation = 0.0f;
    else if (m_Dir == Player::Direction::UP) m_Transform.rotation = 3.14159f;
    else if (m_Dir == Player::Direction::LEFT) m_Transform.rotation = -1.5708f;
    else if (m_Dir == Player::Direction::RIGHT) m_Transform.rotation = 1.5708f;
}

// タ盢 const InteractableManager& im 干把计い
void Turret::Fire(std::vector<std::shared_ptr<Projectile>>& outProjectiles, const LevelManager& lm, const BombManager& bm, const InteractableManager& im, const std::vector<std::shared_ptr<Turret>>& turrets) {
    int dirX = 0, dirY = 0;
    if (m_Dir == Player::Direction::UP) dirY = -1;
    else if (m_Dir == Player::Direction::DOWN) dirY = 1;
    else if (m_Dir == Player::Direction::LEFT) dirX = -1;
    else if (m_Dir == Player::Direction::RIGHT) dirX = 1;

    int targetX = -1;
    int targetY = -1;

    // 眖程环 3 ┕苯磞 2 
    for (int dist = 3; dist >= 2; --dist) {
        int checkX = m_GridX + dirX * dist;
        int checkY = m_GridY + dirY * dist;

        if (checkX >= 0 && checkX < 25 && checkY >= 0 && checkY < 17) {

            bool hasTurret = false;
            for (const auto& t : turrets) {
                if (t->GetGridX() == checkX && t->GetGridY() == checkY) {
                    hasTurret = true;
                    break;
                }
            }

            //  im 竒タ絋肚秈︽﹚
            if (lm.IsWalkable(checkX, checkY) && !lm.IsBrick(checkX, checkY) && !bm.IsBombAt(checkX, checkY) && !hasTurret && !im.IsBlocksBombAt(checkX, checkY)) {
                targetX = checkX;
                targetY = checkY;
                break;
            }
        }
    }

    // 常⊿竚碞ぃ祇甮
    if (targetX == -1 && targetY == -1) {
        return;
    }

    auto bullet = std::make_shared<Projectile>(m_GridX, m_GridY, targetX, targetY, m_Dir);
    outProjectiles.push_back(bullet);
}

RotatingTurret::RotatingTurret(int gridX, int gridY, Player::Direction startDir)
    : Turret(gridX, gridY, startDir) {
}

void RotatingTurret::Rotate() {
    if (m_Dir == Player::Direction::UP) m_Dir = Player::Direction::RIGHT;
    else if (m_Dir == Player::Direction::RIGHT) m_Dir = Player::Direction::DOWN;
    else if (m_Dir == Player::Direction::DOWN) m_Dir = Player::Direction::LEFT;
    else if (m_Dir == Player::Direction::LEFT) m_Dir = Player::Direction::UP;
    UpdateRotationVisual();
}

void RotatingTurret::Update(std::vector<std::shared_ptr<Projectile>>& outProjectiles, const LevelManager& lm, const BombManager& bm, const InteractableManager& im, const std::vector<std::shared_ptr<Turret>>& turrets) {
    m_Timer--;

    if (m_State == State::IDLE) {
        if (m_Timer <= 0) {
            Fire(outProjectiles, lm, bm, im, turrets);

            m_State = State::READY;
            m_Timer = 30;
            SetDrawable(m_ImgActive);
        }
    }
    else if (m_State == State::READY) {
        if (m_Timer <= 0) {
            Rotate();

            m_State = State::IDLE;
            m_Timer = 60 * 5;       // 5 玱
            SetDrawable(m_ImgIdle);
        }
    }
}