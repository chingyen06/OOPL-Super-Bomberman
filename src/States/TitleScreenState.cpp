#include "States/TitleScreenState.hpp"

#include <cmath>

#include "Core/App.hpp"
#include "States/MainMenuState.hpp"
#include "States/MenuCommon.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"
#include "Util/Logger.hpp"

void TitleScreenState::OnEnter(App& app) {
    app.PlayMusic(RESOURCE_DIR"/Sound/title_menu.mp3");
    app.ShowMenuBg();  // cover_bg.png (與選單共用，無 logo/文字)
    auto& root = app.Root();

    m_Logo = std::make_shared<UIImage>(RESOURCE_DIR"/Image/logo.png", 0.0f, kLogoBaseY, 5.0f);
    root.AddChild(m_Logo);

    m_Press = std::make_shared<UIText>("- 請按空白鍵 -", 0.0f, -250.0f, 10.0f, DarkText());
    root.AddChild(m_Press);

    m_Hint = AddKeyHint(app, {{"ESC", "結束遊戲"}});

    m_Tick = 0;

    m_QuitDialog.Init();
    m_Confirming = false;
}

void TitleScreenState::OnExit(App& app) {
    auto& root = app.Root();
    root.RemoveChild(m_Logo);
    root.RemoveChild(m_Press);
    m_Hint.Remove(app);
    if (m_Confirming) { m_QuitDialog.Hide(root); m_Confirming = false; }
    app.HideMenuBg();
}

void TitleScreenState::OnUpdate(App& app) {
    using K = Util::Keycode;
    auto& root = app.Root();

    m_Tick++;
    m_Logo->SetPosition(0.0f, kLogoBaseY + std::sin(m_Tick * 0.09f) * 14.0f);  // 整體上下彈跳

    // 結束確認對話框 (modal)：開啟時凍結其他輸入
    if (m_Confirming) {
        const ConfirmDialog::Result r = m_QuitDialog.Update();
        if (r == ConfirmDialog::Result::Yes) { app.RequestQuit(); return; }
        if (r == ConfirmDialog::Result::No)  { m_QuitDialog.Hide(root); m_Confirming = false; }
        return;
    }
    // ESC → 結束遊戲確認 (與主選單一致)
    if (Util::Input::IsKeyUp(K::ESCAPE)) {
        m_QuitDialog.Show(root, "確定要結束遊戲嗎?");
        m_Confirming = true;
        return;
    }

    if (Util::Input::IsKeyUp(K::SPACE)) {
        LOG_INFO("Enter Main Menu");
        app.TransitionTo(std::make_unique<MainMenuState>());
    }
}
