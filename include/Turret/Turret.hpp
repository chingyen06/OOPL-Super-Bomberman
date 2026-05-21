#ifndef TURRET_HPP
#define TURRET_HPP

#include "Util/GameObject.hpp"
#include "Util/Image.hpp"
#include "Player.hpp"
#include "Turret/Projectile.hpp"
#include <vector>
#include <memory>

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

protected:
    int m_GridX;
    int m_GridY;
    Direction m_Dir;
    int m_Timer;

    std::shared_ptr<Util::Image> m_ImgActive;
    std::shared_ptr<Util::Image> m_ImgIdle;

    void UpdateRotationVisual();

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