#ifndef IDEFENDERWEAPON_HPP
#define IDEFENDERWEAPON_HPP

#include <memory>
#include <vector>

#include "Util/Renderer.hpp"

class Player;
class LevelManager;
class IWeaponEffects;

// 一次「武器發動」所需的全部上下文 (參數包) — 把過去 IDefenderWeapon::Fire 的 5 個獨立
// 參數收斂為一個物件 (Parameter Object)，可讀性提升、未來新增上下文不必改所有子類簽名 (OCP)。
class FireContext {
public:
    FireContext(Player& defender,
                std::vector<std::shared_ptr<Player>>& players,
                LevelManager& level,
                Util::Renderer& root,
                IWeaponEffects& fx)
        : m_Defender(defender), m_Players(players), m_Level(level), m_Root(root), m_Fx(fx) {}

    Player&                                       Defender() const { return m_Defender; }
    std::vector<std::shared_ptr<Player>>&         Players()  const { return m_Players; }
    LevelManager&                                 Level()    const { return m_Level; }
    Util::Renderer&                               Root()     const { return m_Root; }
    IWeaponEffects&                               Fx()       const { return m_Fx; }

private:
    Player&                               m_Defender;
    std::vector<std::shared_ptr<Player>>& m_Players;
    LevelManager&                         m_Level;
    Util::Renderer&                       m_Root;
    IWeaponEffects&                       m_Fx;
};

// 防守方武器介面 (策略模式 / OCP)：新增武器只需實作本介面 + 在工廠加一行。
class IDefenderWeapon {
public:
    virtual ~IDefenderWeapon() = default;

    // 發動武器：對範圍內進攻方造成擊倒、生成特效 (透過 ctx.Fx())、必要時改地形 (透過 ctx.Level())。
    // 回傳本次擊倒的進攻方數 (供充能加成與金幣計算)。
    virtual int Fire(const FireContext& ctx) = 0;
};

#endif
