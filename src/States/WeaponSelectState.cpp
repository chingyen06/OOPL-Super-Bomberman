#include "States/WeaponSelectState.hpp"

#include "Config/MatchConfig.hpp"
#include "Core/App.hpp"
#include "States/BattleSetupState.hpp"
#include "States/MenuCommon.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"

// 三個武器 (順序對應 MatchConfig::Weapon: Sword, Laser, Barrier)
static const char* kWeaponNames[] = { "劍", "雷射砲", "屏障" };
static const char* kWeaponDescs[] = {
    "劍：充能滿後，朝面向揮砍近距離擊倒進攻方。",
    "雷射砲：充能滿後，朝面向發射貫穿雷射。",
    "屏障：充能滿後，在面前生成阻擋進攻方的屏障。",
};

void WeaponSelectState::OnEnter(App& app) {
    app.ShowMenuBg();
    auto& root = app.Root();

    m_Title = std::make_shared<UIText>("選擇武器", -505.0f, 320.0f, 30.0f, MenuCommon::DarkText());
    root.AddChild(m_Title);

    m_List.Init(RESOURCE_DIR"/Image/btn.png", RESOURCE_DIR"/Image/btn_sel.png");
    for (int i = 0; i < MatchConfig::kWeaponCount; ++i) m_List.AddItem(kWeaponNames[i]);
    m_List.Show(root, 0.0f, 120.0f, 0.0f, -90.0f);
    m_List.SetSelected(static_cast<int>(app.Session().Config().DefenderWeapon()));

    m_Desc = std::make_shared<UIText>("-", 0.0f, -150.0f, 30.0f,
                                      Util::Color::FromName(Util::Colors::DIM_GRAY));
    m_Desc->SetScale(0.7f, 0.7f);
    root.AddChild(m_Desc);
    RefreshDesc();

    m_Hint = MenuCommon::AddKeyHint(app, {{"方向鍵", "選擇"}, {"空格鍵", "確定"}, {"X", "返回"}});
}

void WeaponSelectState::OnExit(App& app) {
    m_List.Hide(app.Root());
    app.Root().RemoveChild(m_Title);
    app.Root().RemoveChild(m_Desc);
    m_Hint.Remove(app);
    app.HideMenuBg();
}

void WeaponSelectState::RefreshDesc() {
    const int i = m_List.Selected();
    if (i >= 0 && i < MatchConfig::kWeaponCount) m_Desc->SetText(kWeaponDescs[i]);
}

void WeaponSelectState::OnUpdate(App& app) {
    const int picked = m_List.Update();
    RefreshDesc();  // 隨選取更新說明
    if (picked >= 0) {
        app.Session().Config().SetDefenderWeapon(static_cast<MatchConfig::Weapon>(picked));
        app.TransitionTo(std::make_unique<BattleSetupState>());
        return;
    }
    if (Util::Input::IsKeyUp(Util::Keycode::X)) {
        app.TransitionTo(std::make_unique<BattleSetupState>());
    }
}
