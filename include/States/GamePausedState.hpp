#ifndef GAMEPAUSEDSTATE_HPP
#define GAMEPAUSEDSTATE_HPP

#include "States/IGameState.hpp"

// 暫停：顯示暫停選單；ENTER / X 返回遊戲。暫停時顯示滑鼠。
class GamePausedState : public IGameState {
public:
    void OnEnter(App& app) override;
    void OnExit(App& app) override;
    void OnUpdate(App& app) override;
    bool WantsCursor(App& app) override;
};

#endif
