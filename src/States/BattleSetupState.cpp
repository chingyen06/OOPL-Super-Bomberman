#include "States/BattleSetupState.hpp"

#include <string>

#include "Config/MatchConfig.hpp"
#include "Core/App.hpp"
#include "States/LevelSelectState.hpp"
#include "States/MainMenuState.hpp"
#include "States/MenuCommon.hpp"
#include "States/RulesState.hpp"
#include "States/TeamSelectState.hpp"
#include "States/WeaponSelectState.hpp"
#include "Util/Color.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"

void BattleSetupState::OnEnter(App& app) {
    app.PlayMusic(RESOURCE_DIR"/Sound/castle_intro.mp3");  // 房間 / 對戰前準備配樂
    app.ShowMenuBg();
    auto& root = app.Root();

    m_Title = std::make_shared<UIText>("離線戰鬥", -505.0f, 320.0f, 30.0f, MenuCommon::DarkText());
    root.AddChild(m_Title);

    // 左側導覽
    m_Nav.Init(RESOURCE_DIR"/Image/btn.png", RESOURCE_DIR"/Image/btn_sel.png");
    m_Nav.AddItem("對戰開始");
    m_Nav.AddItem("更換規則");
    m_Nav.AddItem("選擇隊伍");
    m_Nav.AddItem("選擇武器");
    m_Nav.AddItem("選擇關卡");
    m_Nav.Show(root, -470.0f, 180.0f, 0.0f, -72.0f);

    // 成員格 8 格：card0 = 玩家1 (守)；card1..7 = 進攻席位 slot 0..6
    const MatchConfig& cfg = app.Session().Config();
    const float colXs[4] = { -87.0f, 91.0f, 269.0f, 447.0f };
    const float rowYs[2] = { 120.0f, -10.0f };
    for (int i = 0; i < 8; i++) {
        const float x = colXs[i % 4];
        const float y = rowYs[i / 4];
        auto slot = std::make_shared<UIImage>(RESOURCE_DIR"/Image/slot.png", x, y, 20.0f);

        std::string name;
        Util::Color color = MenuCommon::DarkText();
        if (i == 0) {
            name = "玩家 1";
        }
        else {
            const int s = i - 1;  // 進攻席位
            const MatchConfig::SlotMode m = cfg.AttackerSlot(s);
            if (m == MatchConfig::SlotMode::Off) {
                name = "—";
                color = Util::Color::FromName(Util::Colors::GRAY);  // 未加入 → 灰
            }
            else if (m == MatchConfig::SlotMode::Human) {
                name = "玩家 2";
            }
            else {
                name = "電腦";
            }
        }
        auto label = std::make_shared<UIText>(name, x + kLabelXNudge, y - kLabelYNudge, 30.0f, color);
        m_SlotGroup.Add(root, slot);
        m_SlotGroup.Add(root, label);
    }

    m_LevelInfo = std::make_shared<UIText>(std::string("目前關卡：") + App::LevelName(app.SelectedLevel()),
                                           180.0f, -130.0f, 30.0f, MenuCommon::DarkText());
    root.AddChild(m_LevelInfo);

    m_Coins = MenuCommon::AddCoinHud(app);
    m_Hint = MenuCommon::AddKeyHint(app, {{"方向鍵", "選擇"}, {"空白鍵", "確定"}, {"X", "返回"}});
}

void BattleSetupState::OnExit(App& app) {
    auto& root = app.Root();
    m_Nav.Hide(root);
    root.RemoveChild(m_Title);
    root.RemoveChild(m_LevelInfo);
    m_Hint.Remove(app);
    m_Coins.Remove(app);
    m_SlotGroup.Clear(root);
    app.HideMenuBg();
}

void BattleSetupState::OnUpdate(App& app) {
    const int confirmed = m_Nav.Update();
    switch (confirmed) {
        case 0: app.StartMatch(); return;                                          // 對戰開始
        case 1: app.TransitionTo(std::make_unique<RulesState>());        return;   // 更換規則
        case 2: app.TransitionTo(std::make_unique<TeamSelectState>());   return;   // 選擇隊伍
        case 3: app.TransitionTo(std::make_unique<WeaponSelectState>()); return;   // 選擇武器
        case 4: app.TransitionTo(std::make_unique<LevelSelectState>());  return;   // 選擇關卡
        default: break;
    }
    if (Util::Input::IsKeyUp(Util::Keycode::X)) {  // X = 返回
        app.TransitionTo(std::make_unique<MainMenuState>());
    }
}
