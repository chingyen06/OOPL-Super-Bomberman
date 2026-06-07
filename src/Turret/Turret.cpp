#include "Turret/Turret.hpp"
#include "GameConstants.hpp"
#include "GridCoord.hpp"
#include "LevelManager.hpp"
#include "BombManager.hpp"
#include "InteractableManager.hpp"
#include <cmath>

Turret::Turret(int gridX, int gridY, Direction dir)
    : m_GridX(gridX), m_GridY(gridY), m_Dir(dir),
      m_Timer(Constants::Turret::kInitialIdleFrames),
      m_CooldownTotal(Constants::Turret::kInitialIdleFrames) {
    m_ImgActive = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/turret_down.png");
    m_ImgIdle = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/turret_down.png");

    SetDrawable(m_ImgIdle);
    SetZIndex(18);
    m_Transform.translation = GridCoord::ToPixel(gridX, gridY);
    UpdateRotationVisual();

    // 冷卻條：浮在砲台上方；填充由左往右長滿表示即將發射
    const glm::vec2 p = GridCoord::ToPixel(gridX, gridY);
    m_CdBg   = std::make_shared<UIImage>(RESOURCE_DIR"/Image/turret_cd_bg.png", p.x, p.y + 22.0f, 19.0f);
    m_CdFill = std::make_shared<UIImage>(RESOURCE_DIR"/Image/turret_cd.png",    p.x, p.y + 22.0f, 20.0f);
    m_Overlays = { m_CdBg, m_CdFill };
    UpdateCooldownVisual(false);
}

void Turret::UpdateCooldownVisual(bool ready) {
    const float fullW = 32.0f;                       // 背景條寬 (= 圖原寬)
    const glm::vec2 p = GridCoord::ToPixel(m_GridX, m_GridY);
    float prog = ready ? 1.0f
                       : (m_CooldownTotal > 0 ? 1.0f - static_cast<float>(m_Timer) / m_CooldownTotal : 1.0f);
    if (prog < 0.0f) prog = 0.0f; else if (prog > 1.0f) prog = 1.0f;
    // 由左往右長：填充寬 = fullW*prog，靠左對齊背景左緣
    const float left = p.x - fullW * 0.5f;
    m_CdFill->SetScale(prog, 1.0f);
    m_CdFill->SetPosition(left + fullW * prog * 0.5f, p.y + 22.0f);
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

            if (lm.IsWalkable(checkX, checkY) && !lm.IsBrick(checkX, checkY) && !bm.IsBombAt(checkX, checkY) && !hasTurret && !im.IsBlocksBombAt(checkX, checkY)) {
                targetX = checkX;
                targetY = checkY;
                break;
            }
        }
    }

    if (targetX == -1 && targetY == -1) return;

    auto bullet = std::make_shared<Projectile>(m_GridX, m_GridY, targetX, targetY, m_Dir);
    outProjectiles.push_back(bullet);
}

RotatingTurret::RotatingTurret(int gridX, int gridY, Direction startDir)
    : Turret(gridX, gridY, startDir) {
}

void RotatingTurret::Rotate() {
    // 透過基底封裝介面切換方向，不再直接寫 m_Dir
    if (Dir() == Direction::UP) SetDir(Direction::RIGHT);
    else if (Dir() == Direction::RIGHT) SetDir(Direction::DOWN);
    else if (Dir() == Direction::DOWN) SetDir(Direction::LEFT);
    else if (Dir() == Direction::LEFT) SetDir(Direction::UP);
    UpdateRotationVisual();
}

void RotatingTurret::Update(std::vector<std::shared_ptr<Projectile>>& outProjectiles, const LevelManager& lm, const BombManager& bm, const InteractableManager& im, const std::vector<std::shared_ptr<Turret>>& turrets) {
    DecTimer();

    if (m_State == State::IDLE) {
        if (TimerLeft() <= 0) {
            Fire(outProjectiles, lm, bm, im, turrets);

            m_State = State::READY;
            StartPhase(Constants::Turret::kReadyFrames);
            ShowActive();
        }
    }
    else if (m_State == State::READY) {
        if (TimerLeft() <= 0) {
            Rotate();

            m_State = State::IDLE;
            StartPhase(Constants::Turret::kCooldownFrames);
            ShowIdle();
        }
    }

    // 充能條：IDLE(冷卻中) 隨時間長滿；READY(剛發射的待發姿勢) 顯示滿條
    UpdateCooldownVisual(m_State == State::READY);
}
