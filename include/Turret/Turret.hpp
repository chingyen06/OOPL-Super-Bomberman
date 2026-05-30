#ifndef TURRET_HPP
#define TURRET_HPP

#include "Util/GameObject.hpp"
#include "Util/Image.hpp"
#include "UI/UIImage.hpp"
#include "GameTypes.hpp"  // Direction
#include "Turret/Projectile.hpp"
#include <vector>
#include <memory>

class Player;

class LevelManager;
class BombManager;
class InteractableManager;

class Turret : public Util::GameObject {
public:
    Turret(int gridX, int gridY, Direction dir);
    virtual ~Turret() = default;

    virtual void Update(std::vector<std::shared_ptr<Projectile>>& outProjectiles, const LevelManager& lm, const BombManager& bm, const InteractableManager& im, const std::vector<std::shared_ptr<Turret>>& turrets) = 0;

    int GetGridX() const { return m_GridX; }
    int GetGridY() const { return m_GridY; }

    // 冷卻條 (背景 + 充能填充)，由 TurretManager 一併掛上 / 移除場景
    const std::vector<std::shared_ptr<UIImage>>& Overlays() const { return m_Overlays; }

protected:
    int m_GridX;
    int m_GridY;
    Direction m_Dir;
    int m_Timer;
    int m_CooldownTotal;  // 目前這段冷卻/待機的總長 (供充能條換算進度)

    std::shared_ptr<Util::Image> m_ImgActive;
    std::shared_ptr<Util::Image> m_ImgIdle;

    std::shared_ptr<UIImage> m_CdBg;    // 冷卻條背景
    std::shared_ptr<UIImage> m_CdFill;  // 冷卻條填充 (隨充能由左往右長滿 → 快發射)
    std::vector<std::shared_ptr<UIImage>> m_Overlays;

    void UpdateRotationVisual();
    void UpdateCooldownVisual(bool ready);  // ready=true 顯示滿條 (剛發射/待發)

    void Fire(std::vector<std::shared_ptr<Projectile>>& outProjectiles, const LevelManager& lm, const BombManager& bm, const InteractableManager& im, const std::vector<std::shared_ptr<Turret>>& turrets);
};

class RotatingTurret : public Turret {
public:
    enum class State { IDLE, READY };

    RotatingTurret(int gridX, int gridY, Direction startDir);
    void Update(std::vector<std::shared_ptr<Projectile>>& outProjectiles, const LevelManager& lm, const BombManager& bm, const InteractableManager& im, const std::vector<std::shared_ptr<Turret>>& turrets) override;

private:
    State m_State = State::IDLE;
    void Rotate();
};

#endif