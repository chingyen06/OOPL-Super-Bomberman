#ifndef IDEFENDERWEAPON_HPP
#define IDEFENDERWEAPON_HPP

#include <memory>
#include <vector>

#include "Util/Renderer.hpp"

class Player;
class LevelManager;
class IWeaponEffects;

// 防守方武器介面 (策略模式 / OCP)：新增武器只需實作本介面 + 在工廠加一行。
class IDefenderWeapon {
public:
    virtual ~IDefenderWeapon() = default;

    // 發動武器：對範圍內進攻方造成擊倒、生成特效 (透過 fx)、必要時改地形 (透過 level)。
    // 回傳本次擊倒的進攻方數 (供充能加成與金幣計算)。
    virtual int Fire(Player& defender,
                     std::vector<std::shared_ptr<Player>>& players,
                     LevelManager& level,
                     Util::Renderer& root,
                     IWeaponEffects& fx) = 0;
};

#endif
