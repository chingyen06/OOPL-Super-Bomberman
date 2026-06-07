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

// 砲台基底。資料成員全部 private (封裝)，子類透過 protected 介面 (getter / setter / 行為函式)
// 與基底互動。RotatingTurret 不再直接讀寫 m_Dir / m_State 等成員。
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
    // ---- 給子類使用的封裝介面 (取代過去 protected 資料成員直接讀寫) ----
    Direction Dir() const { return m_Dir; }
    void      SetDir(Direction d) { m_Dir = d; }

    int  TimerLeft() const { return m_Timer; }
    void DecTimer()        { --m_Timer; }
    void StartPhase(int totalFrames) { m_Timer = totalFrames; m_CooldownTotal = totalFrames; }

    void ShowActive() { SetDrawable(m_ImgActive); }
    void ShowIdle()   { SetDrawable(m_ImgIdle); }

    void UpdateRotationVisual();
    void UpdateCooldownVisual(bool ready);  // ready=true 顯示滿條 (剛發射/待發)

    void Fire(std::vector<std::shared_ptr<Projectile>>& outProjectiles, const LevelManager& lm, const BombManager& bm, const InteractableManager& im, const std::vector<std::shared_ptr<Turret>>& turrets);

private:
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
