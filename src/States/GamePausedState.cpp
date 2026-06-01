#include "States/GamePausedState.hpp"

#include "Core/App.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"

void GamePausedState::OnEnter(App& app) { app.ShowPauseMenu(); }
void GamePausedState::OnExit (App& app) { app.HidePauseMenu(); }
bool GamePausedState::WantsCursor(App& /*app*/) { return true; }  // 暫停時顯示滑鼠

void GamePausedState::OnUpdate(App& app) {
    // 暫停鍵 (可設定) / X = 返回遊戲；SPACE = 確認，由選單自己處理。
    if (Util::Input::IsKeyUp(app.Keys().pause) ||
        Util::Input::IsKeyUp(Util::Keycode::X)) {
        app.ResumeGame();
        return;
    }
    app.UpdatePauseMenu();
}
