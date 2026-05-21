#include "Turret/Turret.hpp"
#include "GameConstants.hpp"
#include "GridCoord.hpp"
#include "LevelManager.hpp"
#include "BombManager.hpp"
#include "InteractableManager.hpp"
#include <cmath>

Turret::Turret(int gridX, int gridY, Direction dir)
    : m_GridX(gridX), m_GridY(gridY), m_Dir(dir), m_Timer(Constants::Turret::kInitialIdleFrames) {
    m_ImgActive = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/turret_down.png");
    m_ImgIdle = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/turret_down.png");

    SetDrawable(m_ImgIdle);
    SetZIndex(18);
    m_Transform.translation = GridCoord::ToPixel(gridX, gridY);
    UpdateRotationVisual();
}

void Turret::UpdateRotationVisual() {
    if (m_Dir == Direction::DOWN) m_Transform.rotation = 0.0f;
    else if (m_Dir == Direction::UP) m_Transform.rotation = 3.14159f;
    else if (m_Dir == Direction::LEFT) m_Transform.rotation = -1.5708f;
    else if (m_Dir == Direction::RIGHT) m_Transform.rotation = 1.5708f;
}

// Pass the InteractableManager through so we can also exclude tiles blocked by interactables
void Turret::Fire(std::vector<std::shared_ptr<Projectile>>& outProjectiles, const LevelManager& lm, const BombManager& bm, const InteractableManager& im, const std::vector<std::shared_ptr<Turret>>& turrets) {
    int dirX = 0, dirY = 0;
    if (m_Dir == Direction::UP) dirY = -1;
    else if (m_Dir == Direction::DOWN) dirY = 1;
    else if (m_Dir == Direction::LEFT) dirX = -1;
    else if (m_Dir == Direction::RIGHT) dirX = 1;

    int targetX = -1;
    int targetY = -1;

    // Scan from kFireRangeMax tiles back to kFireRangeMin looking for a valid landing spot
    for (int dist = Constants::Turret::kFireRangeMax; dist >= Constants::Turret::kFireRangeMin; --dist) {
        int checkX = m_GridX + dirX * dist;
        int checkY = m_GridY + dirY * dist;

        if (GridCoord::InBounds(checkX, checkY)) {

            bool hasTurret = false;
            for (const auto& t : turrets) {
                if (t->GetGridX() == checkX && t->GetGridY() == checkY) {
                    hasTurret = true;
                    break;
                }
            }

            // The InteractableManager reference is now available, so we can check it here
            if (lm.IsWalkable(checkX, checkY) && !lm.IsBrick(checkX, checkY) && !bm.IsBombAt(checkX, checkY) && !hasTurret && !im.IsBlocksBombAt(checkX, checkY)) {
                targetX = checkX;
                targetY = checkY;
                break;
            }
        }
    }

    // Skip firing if no valid landing tile was found
    if (targetX == -1 && targetY == -1) {
        return;
    }

    auto bullet = std::make_shared<Projectile>(m_GridX, m_GridY, targetX, targetY, m_Dir);
    outProjectiles.push_back(bullet);
}

RotatingTurret::RotatingTurret(int gridX, int gridY, Direction startDir)
    : Turret(gridX, gridY, startDir) {
}

void RotatingTurret::Rotate() {
    if (m_Dir == Direction::UP) m_Dir = Direction::RIGHT;
    else if (m_Dir == Direction::RIGHT) m_Dir = Direction::DOWN;
    else if (m_Dir == Direction::DOWN) m_Dir = Direction::LEFT;
    else if (m_Dir == Direction::LEFT) m_Dir = Direction::UP;
    UpdateRotationVisual();
}

void RotatingTurret::Update(std::vector<std::shared_ptr<Projectile>>& outProjectiles, const LevelManager& lm, const BombManager& bm, const InteractableManager& im, const std::vector<std::shared_ptr<Turret>>& turrets) {
    m_Timer--;

    if (m_State == State::IDLE) {
        if (m_Timer <= 0) {
            Fire(outProjectiles, lm, bm, im, turrets);

            m_State = State::READY;
            m_Timer = Constants::Turret::kReadyFrames;
            SetDrawable(m_ImgActive);
        }
    }
    else if (m_State == State::READY) {
        if (m_Timer <= 0) {
            Rotate();

            m_State = State::IDLE;
            m_Timer = Constants::Turret::kCooldownFrames;
            SetDrawable(m_ImgIdle);
        }
    }
}
