#include "Core/App.hpp"

#include <SDL.h>
#include <imgui.h>

#include <memory>

#include "States/BattleSetupState.hpp"
#include "States/GamePausedState.hpp"
#include "States/GameplayState.hpp"
#include "States/ResultsState.hpp"
#include "States/TitleScreenState.hpp"
#include "UI/UIImage.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"
#include "Util/Logger.hpp"

// App 只負責：狀態機轉場、生命週期、共用資源 (勝利圖/選單背景/暫停選單)、對外服務 API。
// 各畫面邏輯已拆到 States/ 下各自的類別 (SRP)。

void App::TransitionTo(std::unique_ptr<IGameState> next) {
    if (m_CurrentGameState) m_CurrentGameState->OnExit(*this);
    m_CurrentGameState = std::move(next);
    if (m_CurrentGameState) m_CurrentGameState->OnEnter(*this);
}

void App::SetCursorShown(bool shown) {
    if (shown == m_CursorShown) return;  // 僅在改變時呼叫，避免閃爍
    SDL_ShowCursor(shown ? SDL_ENABLE : SDL_DISABLE);
    m_CursorShown = shown;
}

void App::Start() {
    LOG_TRACE("Start");

    m_Save.Load();  // 讀取金幣 / 音量存檔 (關掉重開仍保留)
    m_Music.SetVolume(m_Save.BgmVolume() * 128 / 100);  // 套用存檔的背景音樂音量
    m_Session.SetProfile(&m_Save);      // debug 主控台讀寫金幣用
    m_Session.SetKeyBindings(&m_Keys);  // 套用玩家按鍵設定

    // 讓 ImGui 不要自己改 OS 游標，避免和我們的隱藏邏輯打架而閃爍
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    SDL_ShowCursor(SDL_DISABLE);  // 預設隱藏；只有暫停/Debug 才顯示
    m_CursorShown = false;

    m_DefenseImage = std::make_shared<UIImage>(RESOURCE_DIR"/Image/defense_win.png");  m_DefenseImage->SetFullScreen();
    m_AttackImage  = std::make_shared<UIImage>(RESOURCE_DIR"/Image/attack_win.png");   m_AttackImage->SetFullScreen();
    // 去字的封面彩屑背景：封面與所有選單共用 (z=0，墊在最底層)
    m_MenuBg       = std::make_shared<UIImage>(RESOURCE_DIR"/Image/cover_bg.png", 0.0f, 0.0f, 0.0f); m_MenuBg->SetFullScreen();

    // 暫停選單 (順序需與 kCheatP1Index / kCheatP2Index 對應)
    m_PauseMenu.Init();
    m_PauseMenu.AddOption("繼續",       [this]() { ResumeGame(); });          // index 0
    m_PauseMenu.AddOption("作弊P1：關", [this]() {                            // index 1 == kCheatP1Index
        m_Session.ToggleCheat(0);
        m_PauseMenu.SetOptionLabel(kCheatP1Index,
            m_Session.IsCheatEnabled(0) ? "作弊P1：開" : "作弊P1：關");
    });
    m_PauseMenu.AddOption("作弊P2：關", [this]() {                            // index 2 == kCheatP2Index
        m_Session.ToggleCheat(1);
        m_PauseMenu.SetOptionLabel(kCheatP2Index,
            m_Session.IsCheatEnabled(1) ? "作弊P2：開" : "作弊P2：關");
    });
    m_PauseMenu.AddOption("再次開始",   [this]() { RestartLevel(); });        // index 3
    m_PauseMenu.AddOption("返回房間",   [this]() { ReturnToRoom(); });        // index 4

    m_CurrentState = State::UPDATE;
    TransitionTo(std::make_unique<TitleScreenState>());
}

void App::Update() {
    m_Root.Update();

    if (m_CurrentGameState) m_CurrentGameState->OnUpdate(*this);

    // 依目前狀態決定滑鼠顯示 (change-detection，避免每幀閃爍)
    if (m_CurrentGameState) SetCursorShown(m_CurrentGameState->WantsCursor(*this));

    // 只有視窗關閉鈕會直接結束；ESC 改由主選單的結束確認對話框處理 (其他畫面 ESC 無效)。
    if (Util::Input::IfExit()) {
        m_CurrentState = State::END;
    }

    if (m_CurrentState == State::UPDATE) ValidTask();
}

void App::End() { LOG_TRACE("End"); }

void App::ValidTask() { LOG_TRACE("ValidTask"); }

const char* App::LevelName(int level) {
    switch (level) {
        case 1:  return "炸彈節";
        case 2:  return "植物基地";
        case 3:  return "磐石論壇";
        default: return "未知關卡";
    }
}

void App::StartMatch() {
    m_Music.Play(RESOURCE_DIR"/Sound/battle.mp3");  // 對戰配樂
    TransitionTo(std::make_unique<GameplayState>());
    m_Session.LoadLevel(m_SelectedLevel);
}

void App::EndMatch(bool defenderWin) {
    m_LastResult = m_Session.BuildResult(defenderWin);  // 須在 Clear() 之前
    m_Save.AddCoins(m_LastResult.coinsEarned);          // 發放並存檔
    if (defenderWin) ShowDefenderWin(); else ShowAttackerWin();
    PlayVictorySfx();                                   // 勝利音效 (victory.mp3)
    TransitionTo(std::make_unique<ResultsState>());
}

void App::ShowMenuBg()      { m_Root.AddChild(m_MenuBg); }
void App::HideMenuBg()      { m_Root.RemoveChild(m_MenuBg); }
void App::ShowAttackerWin() { m_Root.AddChild(m_AttackImage); }
void App::ShowDefenderWin() { m_Root.AddChild(m_DefenseImage); }
void App::HideWinScreens() {
    m_Root.RemoveChild(m_AttackImage);
    m_Root.RemoveChild(m_DefenseImage);
}

// ----------------------------- Pause -----------------------------

void App::PauseGame()  { TransitionTo(std::make_unique<GamePausedState>()); }
void App::ResumeGame() { TransitionTo(std::make_unique<GameplayState>()); }

void App::RestartLevel() {
    LOG_INFO("Restarting level");
    m_Music.Play(RESOURCE_DIR"/Sound/battle.mp3");  // 重開仍是對戰配樂 (同曲不重播)
    m_Session.LoadLevel(m_Session.GetCurrentLevel());
    TransitionTo(std::make_unique<GameplayState>());
}

void App::ReturnToRoom() {
    LOG_INFO("Returning to battle setup");
    m_Session.Clear();
    TransitionTo(std::make_unique<BattleSetupState>());
}

void App::ShowPauseMenu() {
    m_PauseMenu.SetOptionLabel(kCheatP1Index,
        m_Session.IsCheatEnabled(0) ? "作弊P1：開" : "作弊P1：關");

    // 玩家2 非人類時：禁用 P2 作弊選項 (變灰、無法選取)
    const bool hasP2 = m_Session.HasHumanPlayer2();
    m_PauseMenu.SetOptionEnabled(kCheatP2Index, hasP2);
    m_PauseMenu.SetOptionLabel(kCheatP2Index,
        !hasP2 ? "作弊P2：無玩家"
               : (m_Session.IsCheatEnabled(1) ? "作弊P2：開" : "作弊P2：關"));

    m_PauseMenu.Show(m_Root);
}
void App::HidePauseMenu()   { m_PauseMenu.Hide(m_Root); }
void App::UpdatePauseMenu() { m_PauseMenu.Update(); }
