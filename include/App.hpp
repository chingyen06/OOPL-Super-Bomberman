#ifndef APP_HPP
#define APP_HPP

#include "pch.hpp" // IWYU pragma: export

#include <memory>
#include "BackgroundImage.hpp"

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

    std::shared_ptr<BackgroundImage> m_CoverImage;  // 存封面圖片的 pointer
};

#endif
