#include "DefenderWeaponSystem.hpp"

#include "GameConstants.hpp"
#include "LevelManager.hpp"
#include "Player.hpp"
#include "Weapons/WeaponFactory.hpp"

void DefenderWeaponSystem::Init(MatchConfig::Weapon weapon, Util::Renderer& root) {
    m_Root = &root;
    m_Weapon = WeaponFactory::Create(weapon);
    m_Charge = 0.0f;
    m_DefenderKills = 0;
    m_ChargeBg   = std::make_shared<UIImage>(RESOURCE_DIR"/Image/charge_bg.png",   -1000.0f, -1000.0f, 86.0f);
    m_ChargeFill = std::make_shared<UIImage>(RESOURCE_DIR"/Image/charge_fill.png", -1000.0f, -1000.0f, 87.0f);
    m_Root->AddChild(m_ChargeBg);
    m_Root->AddChild(m_ChargeFill);
}

void DefenderWeaponSystem::AddEffect(float px, float py, const std::string& sprite, int frames) {
    auto e = std::make_shared<UIImage>(sprite, px, py, 85.0f);
    if (m_Root) m_Root->AddChild(e);
    m_WeaponFx.emplace_back(e, frames);
}

void DefenderWeaponSystem::Update(Player* humanPlayer, std::vector<std::shared_ptr<Player>>& players,
                                  LevelManager& level, bool cheatFullCharge) {
    if (!m_Root) return;
    constexpr float kPerFrame  = 1.0f / (12.0f * Constants::Game::kFPS);  // 約 12 秒充滿
    constexpr float kKillBonus = 0.34f;                                   // 每擊倒 1 名大幅回充

    // 作弊 P1：維持滿格
    if (cheatFullCharge) m_Charge = 1.0f;

    // 時間慢回
    if (!cheatFullCharge && humanPlayer && !humanPlayer->IsDead() && m_Charge < 1.0f) {
        m_Charge += kPerFrame;
        if (m_Charge > 1.0f) m_Charge = 1.0f;
    }
    // 滿了且按發動鍵 → 發動 (暈倒中不能發動武器)，依擊倒回充
    if (m_Weapon && humanPlayer && !humanPlayer->IsDead() && !humanPlayer->IsStunned() && m_Charge >= 1.0f) {
        InputController* ctrl = humanPlayer->GetController();
        if (ctrl && ctrl->IsWeaponJustPressed()) {
            const int kills = m_Weapon->Fire(*humanPlayer, players, level, *m_Root, *this);
            m_DefenderKills += kills;
            float bonus = kills * kKillBonus;
            m_Charge = (bonus > 1.0f) ? 1.0f : bonus;  // 歸零後依擊倒回充
            if (cheatFullCharge) m_Charge = 1.0f;      // 作弊：發動後立即補滿
        }
    }
    // 特效壽命
    for (auto it = m_WeaponFx.begin(); it != m_WeaponFx.end();) {
        if (--it->second <= 0) { m_Root->RemoveChild(it->first); it = m_WeaponFx.erase(it); }
        else ++it;
    }
    // 屏障到期還原地形
    level.TickTemporary(*m_Root);

    // 充能條：浮在防守方頭上，由左往右填滿
    if (m_ChargeBg && m_ChargeFill) {
        if (humanPlayer && !humanPlayer->IsDead()) {
            const glm::vec2 p = humanPlayer->GetPixelPos();
            const float full = 32.0f, by = p.y + 62.0f, left = p.x - full * 0.5f;
            const float c = m_Charge < 0.02f ? 0.02f : m_Charge;
            m_ChargeBg->SetPosition(p.x, by);
            m_ChargeFill->SetScale(c, 1.0f);
            m_ChargeFill->SetPosition(left + full * c * 0.5f, by);
        }
        else {
            m_ChargeBg->SetPosition(-1000.0f, -1000.0f);
            m_ChargeFill->SetPosition(-1000.0f, -1000.0f);
        }
    }
}

void DefenderWeaponSystem::Clear() {
    m_Weapon.reset();
    if (m_Root) {
        if (m_ChargeBg)   { m_Root->RemoveChild(m_ChargeBg);   }
        if (m_ChargeFill) { m_Root->RemoveChild(m_ChargeFill); }
        for (auto& e : m_WeaponFx) m_Root->RemoveChild(e.first);
    }
    m_ChargeBg.reset();
    m_ChargeFill.reset();
    m_WeaponFx.clear();
    m_Charge = 0.0f;
    m_DefenderKills = 0;
}
