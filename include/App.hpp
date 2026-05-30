#ifndef APP_HPP
#define APP_HPP

#include "pch.hpp" // IWYU pragma: export
#include <memory>

#include "GameSession.hpp"
#include "PauseMenu.hpp"
#include "UI/UIImage.hpp"
#include "Util/Renderer.hpp"

class App;

class IGameState {
public:
    virtual ~IGameState() = default;
    virtual void OnEnter(App& /*app*/) {}
    virtual void OnUpdate(App& app) = 0;
    virtual void OnExit(App& /*app*/) {}
};

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

    // 對戰設定：目前選定的關卡 (1..kNumLevels)
    int  SelectedLevel() const { return m_SelectedLevel; }
    void SetSelectedLevel(int level) { m_SelectedLevel = level; }
    static constexpr int NumLevels() { return kNumLevels; }

    void StartMatch();  // 載入選定關卡並進入遊戲

    // overlay 操作 (封面 / 選單背景 / 勝負畫面)
    void ShowCover();
    void HideCover();
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

private:
    void ValidTask();

    State m_CurrentState = State::START;
    std::unique_ptr<IGameState> m_CurrentGameState;

    // m_Root 必須在 m_Session 之前宣告，因 m_Session 以 m_Root 初始化
    Util::Renderer m_Root;
    GameSession    m_Session{ m_Root };

    std::shared_ptr<UIImage> m_CoverImage;
    std::shared_ptr<UIImage> m_DefenseImage;
    std::shared_ptr<UIImage> m_AttackImage;
    std::shared_ptr<UIImage> m_MenuBg;

    PauseMenu m_PauseMenu;

    int m_SelectedLevel = 1;

    static constexpr int kNumLevels        = 3;
    static constexpr int kCheatOptionIndex = 1;  // 暫停選單中「作弊模式」選項的索引
};

#endif
