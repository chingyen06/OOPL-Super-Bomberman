#include "Turret/Turret.hpp"
#include <cmath>

Turret::Turret(int gridX, int gridY, Player::Direction dir)
    : m_GridX(gridX), m_GridY(gridY), m_Dir(dir), m_Timer(60) {
    m_ImgActive = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/turret_down.png");
    m_ImgIdle = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/turret_down.png");

    SetDrawable(m_ImgIdle);
    SetZIndex(5);
    m_Transform.translation = { (gridX - 12) * 32.0f, (8 - gridY) * 32.0f };
    UpdateRotationVisual();
}

void Turret::UpdateRotationVisual() {
    if (m_Dir == Player::Direction::UP) m_Transform.rotation = 0.0f;
    else if (m_Dir == Player::Direction::DOWN) m_Transform.rotation = 3.14159f;
    else if (m_Dir == Player::Direction::LEFT) m_Transform.rotation = 1.5708f;
    else if (m_Dir == Player::Direction::RIGHT) m_Transform.rotation = -1.5708f;
}

void Turret::Fire(std::vector<std::shared_ptr<Projectile>>& outProjectiles) {
    auto bullet = std::make_shared<Projectile>(m_GridX, m_GridY, m_Dir);
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

void RotatingTurret::Update(std::vector<std::shared_ptr<Projectile>>& outProjectiles) {
    m_Timer--;

    if (m_State == State::IDLE) {
        if (m_Timer <= 0) {
            Rotate();
            m_State = State::READY;
            m_Timer = 30;
            SetDrawable(m_ImgActive);
        }
    }
    else if (m_State == State::READY) {
        if (m_Timer <= 0) {
            Fire(outProjectiles);
            m_State = State::IDLE;
            m_Timer = 60 * 5;
            SetDrawable(m_ImgIdle);
        }
    }
}