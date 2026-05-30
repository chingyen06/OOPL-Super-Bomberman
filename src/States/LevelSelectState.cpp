#include "States/LevelSelectState.hpp"

#include <string>

#include "Core/App.hpp"
#include "States/BattleSetupState.hpp"
#include "States/MenuCommon.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"

void LevelSelectState::OnEnter(App& app) {
    app.ShowMenuBg();
    auto& root = app.Root();

    m_Title = std::make_shared<UIText>("選擇關卡", -505.0f, 320.0f, 30.0f, DarkText());
    root.AddChild(m_Title);

    m_Thumbs.Init(RESOURCE_DIR"/Image/thumb.png", RESOURCE_DIR"/Image/thumb_sel.png");
    for (int i = 1; i <= App::NumLevels(); i++) {
        m_Thumbs.AddItem("關卡 " + std::to_string(i));
    }
    m_Thumbs.Show(root, -300.0f, 30.0f, 300.0f, 0.0f);
    m_Thumbs.SetSelected(app.SelectedLevel() - 1);  // 對齊目前選擇

    m_Hint = AddKeyHint(app, {{"方向鍵", "選擇"}, {"空格鍵", "確定"}, {"X", "返回"}});
}

void LevelSelectState::OnExit(App& app) {
    m_Thumbs.Hide(app.Root());
    app.Root().RemoveChild(m_Title);
    m_Hint.Remove(app);
    app.HideMenuBg();
}

void LevelSelectState::OnUpdate(App& app) {
    const int picked = m_Thumbs.Update();
    if (picked >= 0) {
        app.SetSelectedLevel(picked + 1);
        app.TransitionTo(std::make_unique<BattleSetupState>());
        return;
    }
    if (Util::Input::IsKeyUp(Util::Keycode::X)) {  // X = 返回
        app.TransitionTo(std::make_unique<BattleSetupState>());
    }
}
