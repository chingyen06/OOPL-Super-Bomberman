#include "States/GameplayState.hpp"

#include "Core/App.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"
#include "Util/Logger.hpp"

bool GameplayState::WantsCursor(App& app) {
    return app.Session().IsDebugOpen();  // 僅 debug 主控台開啟時顯示滑鼠
}

void GameplayState::OnUpdate(App& app) {
    // ENTER 進入暫停 (凍結這一幀之後的所有 gameplay 更新)
    if (Util::Input::IsKeyUp(Util::Keycode::RETURN) || Util::Input::IsKeyUp(Util::Keycode::KP_ENTER)) {
        app.PauseGame();
        return;
    }

    auto& session = app.Session();
    session.Update();

    if (session.IsAttackerWin()) {
        LOG_INFO("Attacker Wins!");
        app.EndMatch(false);  // 防守方(玩家1)落敗
        return;
    }
    if (session.IsTimeUp()) {
        LOG_INFO("Defender Wins!");
        app.EndMatch(true);
    }
}
