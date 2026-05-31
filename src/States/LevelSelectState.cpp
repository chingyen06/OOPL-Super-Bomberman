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

    // 各關縮圖預載 + 大張預覽 (置中上方)
    m_PreviewImgs.clear();
    for (int i = 1; i <= App::NumLevels(); i++) {
        m_PreviewImgs.push_back(std::make_shared<Util::Image>(
            RESOURCE_DIR"/Image/thumb_lv" + std::to_string(i) + ".png"));
    }
    m_Preview = std::make_shared<UIImage>(0.0f, 95.0f, 20.0f);
    m_Preview->SetScale(1.3f, 1.3f);
    root.AddChild(m_Preview);

    m_Thumbs.Init(RESOURCE_DIR"/Image/thumb.png", RESOURCE_DIR"/Image/thumb_sel.png");
    for (int i = 1; i <= App::NumLevels(); i++) {
        m_Thumbs.AddItem(App::LevelName(i));
    }
    m_Thumbs.Show(root, -300.0f, -160.0f, 300.0f, 0.0f);
    m_Thumbs.SetSelected(app.SelectedLevel() - 1);  // 對齊目前選擇

    m_LastSel = -1;  // 強制首幀更新預覽

    m_Hint = AddKeyHint(app, {{"方向鍵", "選擇"}, {"空格鍵", "確定"}, {"X", "返回"}});
}

void LevelSelectState::OnExit(App& app) {
    m_Thumbs.Hide(app.Root());
    app.Root().RemoveChild(m_Title);
    app.Root().RemoveChild(m_Preview);
    m_PreviewImgs.clear();
    m_Hint.Remove(app);
    app.HideMenuBg();
}

void LevelSelectState::OnUpdate(App& app) {
    const int picked = m_Thumbs.Update();

    // 選取改變 → 換預覽圖
    const int sel = m_Thumbs.Selected();
    if (sel != m_LastSel && sel >= 0 && sel < static_cast<int>(m_PreviewImgs.size())) {
        m_Preview->SetDrawable(m_PreviewImgs[sel]);
        m_LastSel = sel;
    }

    if (picked >= 0) {
        app.SetSelectedLevel(picked + 1);
        app.TransitionTo(std::make_unique<BattleSetupState>());
        return;
    }
    if (Util::Input::IsKeyUp(Util::Keycode::X)) {  // X = 返回
        app.TransitionTo(std::make_unique<BattleSetupState>());
    }
}
