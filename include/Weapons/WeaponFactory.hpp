#ifndef WEAPONFACTORY_HPP
#define WEAPONFACTORY_HPP

#include <memory>

#include "Config/MatchConfig.hpp"
#include "Weapons/BarrierWeapon.hpp"
#include "Weapons/IDefenderWeapon.hpp"
#include "Weapons/LaserWeapon.hpp"
#include "Weapons/SwordWeapon.hpp"

// 依 MatchConfig 選擇建立對應武器 (OCP：新增武器在此加一個 case)。
class WeaponFactory {
public:
    static std::unique_ptr<IDefenderWeapon> Create(MatchConfig::Weapon w) {
        switch (w) {
            case MatchConfig::Weapon::Laser:   return std::make_unique<LaserWeapon>();
            case MatchConfig::Weapon::Barrier: return std::make_unique<BarrierWeapon>();
            case MatchConfig::Weapon::Sword:
            default:                           return std::make_unique<SwordWeapon>();
        }
    }
};

#endif
