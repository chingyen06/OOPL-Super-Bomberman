#ifndef BARRIERWEAPON_HPP
#define BARRIERWEAPON_HPP

#include "Weapons/IDefenderWeapon.hpp"

// 屏障：在正前方 3 格生成暫時的不可穿越牆，阻擋進攻方 (不造成擊倒)。
class BarrierWeapon : public IDefenderWeapon {
public:
    int Fire(Player& defender, std::vector<std::shared_ptr<Player>>& players,
             LevelManager& level, Util::Renderer& root, IWeaponEffects& fx) override;
};

#endif
