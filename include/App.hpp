#ifndef APP_HPP
#define APP_HPP

#include "pch.hpp" // IWYU pragma: export
#include <memory>

#include "Util/Renderer.hpp"
#include "BackgroundImage.hpp"
#include "LevelManager.hpp"
#include "Player.hpp"

class App {
public:
    enum class State {
        START,
        UPDATE,
        END,
    };

    State GetCurrentState() const { return m_CurrentState; }

    void Start();

    void Update();

    void End(); // NOLINT(readability-convert-member-functions-to-static)

private:
    void ValidTask();

private:
    enum class GameState {
        TITLE_SCREEN,
        GAMEPLAY
    };

    State m_CurrentState = State::START;
    GameState m_GameState = GameState::TITLE_SCREEN;  // 初始為 TITLE_SCREEN (封面)

    Util::Renderer m_Root;  // 場景的根節點

    std::shared_ptr<BackgroundImage> m_CoverImage;  // 存封面圖片的 pointer
    LevelManager m_LevelManager;  // 管理關卡

    std::shared_ptr<Player> m_Player;  // 存角色的 pointer
};

#endif
