#ifndef GAMEPLAYSTATE_HPP
#define GAMEPLAYSTATE_HPP

#include "States/IGameState.hpp"

// 遊戲進行中：推進 GameSession，偵測勝負並結束對戰；ENTER 進入暫停。
class GameplayState : public IGameState {
public:
    void OnUpdate(App& app) override;
    bool WantsCursor(App& app) override;  // 僅 debug 主控台開啟時顯示滑鼠
};

#endif
