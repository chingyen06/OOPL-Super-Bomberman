#ifndef APP_HPP
#define APP_HPP

#include "pch.hpp" // IWYU pragma: export
#include <memory>

#include "Audio/MusicPlayer.hpp"
#include "Audio/SfxPlayer.hpp"
#include "GameSession.hpp"
#include "KeyBindings.hpp"
#include "PauseMenu.hpp"
#include "SaveData.hpp"
#include "States/IGameState.hpp"
#include "UI/UIImage.hpp"
#include "Util/Renderer.hpp"

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

    void TransitionTo(std::unique_ptr<IGameState> nextState);

    // ------- State 用的 public API -------

    // 選單畫面各自建立 / 移除自己的 UI，需要直接拿到 root (取代舊的逐一封裝)。
    Util::Renderer& Root() { return m_Root; }
    GameSession&    Session() { return m_Session; }
    SaveData&       Profile() { return m_Save; }              // 金幣存檔
    KeyBindings&    Keys() { return m_Keys; }                 // 玩家按鍵設定

    // 對戰結束：產生結算資料、發放金幣、顯示勝利圖並進入結算畫面。
    void EndMatch(bool defenderWin);
    const MatchResult& LastResult() const { return m_LastResult; }

    // 對戰設定：目前選定的關卡 (1..kNumLevels)
    int  SelectedLevel() const { return m_SelectedLevel; }
    void SetSelectedLevel(int level) { m_SelectedLevel = level; }
    static constexpr int NumLevels() { return kNumLevels; }
    static const char* LevelName(int level);  // 1=炸彈節 2=植物基地 3=磐石論壇

    void StartMatch();  // 載入選定關卡並進入遊戲

    // overlay 操作 (選單背景 / 勝負畫面)
    void ShowMenuBg();
    void HideMenuBg();
    void ShowAttackerWin();
    void ShowDefenderWin();
    void HideWinScreens();

    // ------- 暫停選單 -------
    void PauseGame();
    void ResumeGame();
    void RestartLevel();
    void ReturnToRoom();
    void ShowPauseMenu();
    void HidePauseMenu();
    void UpdatePauseMenu();

    void RequestQuit() { m_CurrentState = State::END; }

    // 背景音樂：各畫面在 OnEnter 指定自己的曲目；同一首則不會被重頭播。
    void PlayMusic(const std::string& path) { m_Music.Play(path); }

    // 三組音量 (0..100%)：由「聲音設定」畫面調整，立即套用並存檔 (與金幣一樣持久化)。
    int  BgmVolume()   const { return m_Save.BgmVolume(); }
    int  SfxVolume()   const { return m_Save.SfxVolume(); }
    int  VoiceVolume() const { return m_Save.VoiceVolume(); }
    void SetBgmVolume(int percent) {
        m_Save.SetBgmVolume(percent);
        m_Music.SetVolume(m_Save.BgmVolume() * 128 / 100);  // 0..100% → 0..128
    }
    void SetSfxVolume(int percent)   { m_Save.SetSfxVolume(percent); }    // 套用於下次播放的音效
    void SetVoiceVolume(int percent) { m_Save.SetVoiceVolume(percent); }  // 語音 (目前保留)

    // 勝利音效 (victory.mp3)，依目前 SFX 音量播放一次。
    void PlayVictorySfx() { m_Sfx.Play(RESOURCE_DIR"/Sound/victory.mp3", m_Save.SfxVolume() * 128 / 100); }

    // 只有在狀態改變時才呼叫 SDL_ShowCursor，避免每幀切換造成閃爍
    void SetCursorShown(bool shown);

private:
    void ValidTask();

    bool m_CursorShown = true;

    State m_CurrentState = State::START;
    std::unique_ptr<IGameState> m_CurrentGameState;

    // m_Root 必須在 m_Session 之前宣告，因 m_Session 以 m_Root 初始化
    Util::Renderer m_Root;
    GameSession    m_Session{ m_Root };

    std::shared_ptr<UIImage> m_DefenseImage;
    std::shared_ptr<UIImage> m_AttackImage;
    std::shared_ptr<UIImage> m_MenuBg;

    PauseMenu m_PauseMenu;
    MusicPlayer m_Music;  // 背景音樂 (依畫面切換曲目)
    SfxPlayer   m_Sfx;    // 一次性音效 (勝利等)

    SaveData    m_Save;          // 金幣存檔 (啟動時 Load)
    KeyBindings m_Keys;          // 玩家按鍵設定 (設定畫面可改)
    MatchResult m_LastResult;    // 最近一場結算 (結算畫面顯示用)

    int m_SelectedLevel = 1;

    static constexpr int kNumLevels    = 3;
    static constexpr int kCheatP1Index = 1;  // 暫停選單「作弊模式 P1」選項索引
    static constexpr int kCheatP2Index = 2;  // 暫停選單「作弊模式 P2」選項索引
};

#endif
