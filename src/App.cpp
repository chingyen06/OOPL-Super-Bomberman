#include "App.hpp"

#include "Util/Image.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"
#include "Util/Logger.hpp"

void App::Start() {
    LOG_TRACE("Start");

    m_CoverImage = std::make_shared<BackgroundImage>();  // 載入封面圖片
    m_Root.AddChild(m_CoverImage);  // 將封面圖片加入根節點
    m_LevelManager.LoadLevel(RESOURCE_DIR"/Map/level_1.txt");  // 預先載入第一關

    m_CurrentState = State::UPDATE;
}

void App::Update() {
    m_Root.Update();  // 更新場景

    if (m_GameState == GameState::TITLE_SCREEN) {  // 如果在 TITLE_SCREEN (封面)
        // m_CoverImage->Draw();  // 繪製封面圖片 (用 Renderer 繪圖，不需要這一行)

        if (Util::Input::IsKeyPressed(Util::Keycode::SPACE)) {  // 偵測空白鍵
            LOG_INFO("Start Game");
            m_GameState = GameState::GAMEPLAY;  // 切換到 GAMEPLAY (遊戲)

            m_Root.RemoveChild(m_CoverImage);       // 移除封面圖片
            m_LevelManager.AttachToRoot(m_Root);    // 載入地圖方塊
        }
    }
    else if (m_GameState == GameState::GAMEPLAY) {  // 如果在 GAMEPLAY (遊戲)

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
