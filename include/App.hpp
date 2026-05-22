#ifndef APP_HPP
#define APP_HPP

#include "pch.hpp" // IWYU pragma: export
#include <memory>

#include "GameSession.hpp"
#include "Menu.hpp"
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

    // ------- State 用的 public API (不再用 friend 開後門) -------

    GameSession& Session() { return m_Session; }
    Menu&        MainMenu()  { return m_MainMenu; }
    Menu&        LevelMenu() { return m_LevelMenu; }

    // overlay 操作 (UI 圖片 add/remove 全部封裝在 App 內，State 不直接碰 Renderer)
    void ShowCover();
    void HideCover();
    void ShowMenuBg();
    void HideMenuBg();
    void ShowAttackerWin();
    void ShowDefenderWin();
    void HideWinScreens();

    // Menu show/hide 也封裝起來，State 不需要拿 Renderer
    void ShowMenu(Menu& menu, float x, float y);
    void HideMenu(Menu& menu);

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

    Menu m_MainMenu;
    Menu m_LevelMenu;
};

#endif
