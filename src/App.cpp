#include "App.hpp"

#include "Util/Image.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"
#include "Util/Logger.hpp"

void App::Start() {
    LOG_TRACE("Start");

    m_CoverImage = std::make_shared<BackgroundImage>();

    m_CurrentState = State::UPDATE;
}

void App::Update() {
    if (m_GameState == GameState::TITLE_SCREEN) {  // 如果在 TITLE_SCREEN (封面)
        m_CoverImage->Draw();  // 繪製封面圖片

        if (Util::Input::IsKeyPressed(Util::Keycode::SPACE)) {  // 偵測空白鍵
            LOG_INFO("Start Game");
            m_GameState = GameState::GAMEPLAY;  // 切換到 GAMEPLAY (遊戲)
        }
    }
    
    /*
     * Do not touch the code below as they serve the purpose for
     * closing the window.
     */
    if (Util::Input::IsKeyUp(Util::Keycode::ESCAPE) ||
        Util::Input::IfExit()) {
        m_CurrentState = State::END;
    }
}

void App::End() { // NOLINT(this method will mutate members in the future)
    LOG_TRACE("End");
}
