#ifndef LASERWEAPON_HPP
#define LASERWEAPON_HPP

#include "Weapons/IDefenderWeapon.hpp"

// 雷射砲：朝面向發射貫穿雷射，擊倒一直線上的進攻方，直到撞牆/磚為止。
class LaserWeapon : public IDefenderWeapon {
public:
    int Fire(Player& defender, std::vector<std::shared_ptr<Player>>& players,
             LevelManager& level, Util::Renderer& root, IWeaponEffects& fx) override;
};

#endif
