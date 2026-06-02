#ifndef DEFENDERWEAPONSYSTEM_HPP
#define DEFENDERWEAPONSYSTEM_HPP

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "MatchConfig.hpp"
#include "UI/UIImage.hpp"
#include "Util/Renderer.hpp"
#include "Weapons/IDefenderWeapon.hpp"
#include "Weapons/IWeaponEffects.hpp"

class Player;
class LevelManager;

// 防守方武器子系統 (自 GameSession 抽出，SRP)：武器建立/充能/發動/特效 + 充能條 UI。
// 同時是 IWeaponEffects 實作端，武器透過它生成特效 (DIP)。
class DefenderWeaponSystem : public IWeaponEffects {
public:
    // 建立本場武器與充能條並掛上場景；root 為非擁有指標。
    void Init(MatchConfig::Weapon weapon, Util::Renderer& root);

    // 每幀推進：充能 / 發動 / 特效壽命 / 屏障還原 / 充能條位置。cheatFullCharge 時維持滿格。
    void Update(Player* humanPlayer, std::vector<std::shared_ptr<Player>>& players,
                LevelManager& level, bool cheatFullCharge);

    // 移除武器、充能條與殘留特效 (GameEnd / 換關用)。
    void Clear();

    int DefenderKills() const { return m_DefenderKills; }

    void DebugFillCharge() { m_Charge = 1.0f; }  // debug：立即充滿

    // IWeaponEffects：在像素座標生成壽命 frames 的特效圖。
    void AddEffect(float px, float py, const std::string& sprite, int frames) override;

private:
    Util::Renderer* m_Root = nullptr;              // 非擁有 (Init 注入)
    std::unique_ptr<IDefenderWeapon> m_Weapon;     // 本場武器 (工廠建立)
    float m_Charge        = 0.0f;                  // 充能 0..1
    int   m_DefenderKills = 0;                     // 武器擊倒進攻方累計
    std::shared_ptr<UIImage> m_ChargeBg;           // 充能條背景
    std::shared_ptr<UIImage> m_ChargeFill;         // 充能條填充
    std::vector<std::pair<std::shared_ptr<UIImage>, int>> m_WeaponFx;  // 特效 + 剩餘 frame
};

#endif
