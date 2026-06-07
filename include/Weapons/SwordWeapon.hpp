#ifndef SWORDWEAPON_HPP
#define SWORDWEAPON_HPP

#include "Weapons/IDefenderWeapon.hpp"

// 劍：朝面向揮砍，擊倒正前方 3 格 (前方 + 左右側) 的進攻方。近距離、即時。
class SwordWeapon : public IDefenderWeapon {
public:
    int Fire(const FireContext& ctx) override;
};

#endif
