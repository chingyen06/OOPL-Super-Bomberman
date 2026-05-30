#include "States/TitleScreenState.hpp"

#include <cmath>

#include "Core/App.hpp"
#include "States/MainMenuState.hpp"
#include "States/MenuCommon.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"
#include "Util/Logger.hpp"

void TitleScreenState::OnEnter(App& app) {
    app.ShowMenuBg();  // cover_bg.png (與選單共用，無 logo/文字)
    auto& root = app.Root();

    m_Logo = std::make_shared<UIImage>(RESOURCE_DIR"/Image/logo.png", 0.0f, kLogoBaseY, 5.0f);
    root.AddChild(m_Logo);

    m_Press = std::make_shared<UIText>("- 請按空白鍵 -", 0.0f, -250.0f, 10.0f, DarkText());
    root.AddChild(m_Press);

    m_Tick = 0;
}

void TitleScreenState::OnExit(App& app) {
    auto& root = app.Root();
    root.RemoveChild(m_Logo);
    root.RemoveChild(m_Press);
    app.HideMenuBg();
}

void TitleScreenState::OnUpdate(App& app) {
    m_Tick++;
    m_Logo->SetPosition(0.0f, kLogoBaseY + std::sin(m_Tick * 0.09f) * 14.0f);  // 整體上下彈跳

    if (Util::Input::IsKeyUp(Util::Keycode::SPACE)) {
        LOG_INFO("Enter Main Menu");
        app.TransitionTo(std::make_unique<MainMenuState>());
    }
}
