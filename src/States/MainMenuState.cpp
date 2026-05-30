#include "States/MainMenuState.hpp"

#include "Core/App.hpp"
#include "States/BattleSetupState.hpp"
#include "States/MenuCommon.hpp"
#include "States/SettingsState.hpp"
#include "Util/Color.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"

void MainMenuState::OnEnter(App& app) {
    app.ShowMenuBg();
    auto& root = app.Root();

    m_Title = std::make_shared<UIText>("SUPER BOMBERMAN", 0.0f, 250.0f, 30.0f, DarkText());
    root.AddChild(m_Title);
    m_Subtitle = std::make_shared<UIText>("城堡模式", 0.0f, 195.0f, 30.0f,
                                          Util::Color::FromName(Util::Colors::DIM_GRAY));
    root.AddChild(m_Subtitle);

    // 功能選單 (開始對戰 / 離開遊戲)；設定改由右下角齒輪進入 (按 → 選取)
    m_Menu.Init(RESOURCE_DIR"/Image/btn.png", RESOURCE_DIR"/Image/btn_sel.png");
    m_Menu.AddItem("開始對戰");
    m_Menu.AddItem("離開遊戲");
    m_Menu.Show(root, 0.0f, 40.0f, 0.0f, -100.0f);

    // 齒輪圖示 (右下角)；上移，避免選取放大時卡到下緣提示
    m_Gear = std::make_shared<UIImage>(RESOURCE_DIR"/Image/gear.png", 565.0f, -245.0f, 40.0f);
    root.AddChild(m_Gear);
    m_GearFocused = false;
    UpdateGearVisual();

    m_Coins = AddCoinHud(app);
    m_Hint = AddKeyHint(app, {{"方向鍵", "選擇"}, {"空格鍵", "確定"}, {"ESC", "離開"}});
}

void MainMenuState::OnExit(App& app) {
    auto& root = app.Root();
    m_Menu.Hide(root);
    root.RemoveChild(m_Title);
    root.RemoveChild(m_Subtitle);
    root.RemoveChild(m_Gear);
    m_Hint.Remove(app);
    m_Coins.Remove(app);
    app.HideMenuBg();
}

void MainMenuState::UpdateGearVisual() {
    const float s = m_GearFocused ? 1.3f : 1.0f;  // 選到齒輪時放大標示
    m_Gear->SetScale(s, s);
    m_Menu.SetHighlight(!m_GearFocused);  // 焦點在齒輪時，左側選單不顯示橘色高亮
}

void MainMenuState::OnUpdate(App& app) {
    using K = Util::Keycode;
    if (m_GearFocused) {
        // 焦點在齒輪：← 回到選單；空白鍵進入設定
        if (Util::Input::IsKeyUp(K::LEFT) || Util::Input::IsKeyUp(K::A)) {
            m_GearFocused = false; UpdateGearVisual(); return;
        }
        if (Util::Input::IsKeyUp(K::SPACE)) {
            app.TransitionTo(std::make_unique<SettingsState>()); return;
        }
        return;  // 凍結選單導覽
    }
    // → 把焦點移到右下角齒輪
    if (Util::Input::IsKeyUp(K::RIGHT) || Util::Input::IsKeyUp(K::D)) {
        m_GearFocused = true; UpdateGearVisual(); return;
    }
    const int confirmed = m_Menu.Update();
    if (confirmed == 0) { app.TransitionTo(std::make_unique<BattleSetupState>()); return; }  // 開始對戰
    if (confirmed == 1) { app.RequestQuit(); return; }  // 離開遊戲 (ESC 亦可離開)
}
