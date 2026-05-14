#ifndef TURRET_HPP
#define TURRET_HPP

#include "Util/GameObject.hpp"
#include "Util/Image.hpp"
#include "Player.hpp"
#include "Turret/Projectile.hpp"
#include <vector>
#include <memory>

class Turret : public Util::GameObject {
public:
    Turret(int gridX, int gridY, Player::Direction dir);
    virtual ~Turret() = default;

    virtual void Update(std::vector<std::shared_ptr<Projectile>>& outProjectiles) = 0;

protected:
    int m_GridX;
    int m_GridY;
    Player::Direction m_Dir;
    int m_Timer;

    std::shared_ptr<Util::Image> m_ImgActive;
    std::shared_ptr<Util::Image> m_ImgIdle;

    void UpdateRotationVisual();
    void Fire(std::vector<std::shared_ptr<Projectile>>& outProjectiles);
};

class RotatingTurret : public Turret {
public:
    enum class State { IDLE, READY };

    RotatingTurret(int gridX, int gridY, Player::Direction startDir);
    void Update(std::vector<std::shared_ptr<Projectile>>& outProjectiles) override;

private:
    State m_State = State::IDLE;
    void Rotate();
};

#endif