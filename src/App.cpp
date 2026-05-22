#include "App.hpp"

#include <memory>
#include <string>

#include "Util/Image.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"
#include "Util/Logger.hpp"

// ----------------------------- State forward decls -----------------------------

class TitleScreenState;
class LevelSelectState;
class GameplayState;
class GameEndState;

class TitleScreenState : public IGameState {
public:
    void OnEnter(App& app) override { app.ShowCover(); }
    void OnExit (App& app) override { app.HideCover(); }
    void OnUpdate(App& app) override;
};

class LevelSelectState : public IGameState {
public:
    void OnEnter(App& app) override {
        app.ShowMenuBg();
        app.ShowMenu(app.MainMenu(), 0, 50);
    }
    void OnUpdate(App& app) override {
        if (app.MainMenu().IsVisible())       app.MainMenu().Update();
        else if (app.LevelMenu().IsVisible()) app.LevelMenu().Update();
    }
    void OnExit(App& app) override {
        app.HideMenuBg();
        if (app.MainMenu().IsVisible())  app.HideMenu(app.MainMenu());
        if (app.LevelMenu().IsVisible()) app.HideMenu(app.LevelMenu());
    }
};

class GameplayState : public IGameState {
public:
    void OnUpdate(App& app) override;
};

class GameEndState : public IGameState {
public:
    void OnEnter(App& app) override { app.Session().Clear(); }
    void OnExit (App& app) override { app.HideWinScreens(); }
    void OnUpdate(App& app) override {
        if (Util::Input::IsKeyUp(Util::Keycode::SPACE)) {
            LOG_INFO("Return to Title Screen");
            app.TransitionTo(std::make_unique<TitleScreenState>());
        }
    }
};

void TitleScreenState::OnUpdate(App& app) {
    if (Util::Input::IsKeyUp(Util::Keycode::SPACE)) {
        LOG_INFO("Enter Level Select");
        app.TransitionTo(std::make_unique<LevelSelectState>());
    }
}

void GameplayState::OnUpdate(App& app) {
    auto& session = app.Session();
    session.Update();

    if (session.IsAttackerWin()) {
        LOG_INFO("Attacker Wins!");
        app.ShowAttackerWin();
        app.TransitionTo(std::make_unique<GameEndState>());
        return;
    }
    if (session.IsTimeUp()) {
        LOG_INFO("Defender Wins!");
        app.ShowDefenderWin();
        app.TransitionTo(std::make_unique<GameEndState>());
    }
}

// ----------------------------- App -----------------------------

void App::TransitionTo(std::unique_ptr<IGameState> next) {
    if (m_CurrentGameState) m_CurrentGameState->OnExit(*this);
    m_CurrentGameState = std::move(next);
    if (m_CurrentGameState) m_CurrentGameState->OnEnter(*this);
}

void App::Start() {
    LOG_TRACE("Start");

    m_CoverImage   = std::make_shared<UIImage>(RESOURCE_DIR"/Image/cover.jpg");        m_CoverImage->SetFullScreen();
    m_DefenseImage = std::make_shared<UIImage>(RESOURCE_DIR"/Image/defense_win.png");  m_DefenseImage->SetFullScreen();
    m_AttackImage  = std::make_shared<UIImage>(RESOURCE_DIR"/Image/attack_win.png");   m_AttackImage->SetFullScreen();
    m_MenuBg       = std::make_shared<UIImage>(RESOURCE_DIR"/Image/white.png");        m_MenuBg->SetFullScreen();

    m_MainMenu.AddOption("Start Game", [this]() {
        HideMenu(m_MainMenu);
        ShowMenu(m_LevelMenu, 0, 50);
    });
    m_MainMenu.AddOption("Exit Game", [this]() { RequestQuit(); });

    constexpr int kNumLevels = 3;
    for (int i = 1; i <= kNumLevels; ++i) {
        m_LevelMenu.AddOption("Level " + std::to_string(i), [this, i]() {
            TransitionTo(std::make_unique<GameplayState>());
            m_Session.LoadLevel(i);
        });
    }
    m_LevelMenu.AddOption("return", [this]() {
        TransitionTo(std::make_unique<LevelSelectState>());
    });

    m_CurrentState = State::UPDATE;
    TransitionTo(std::make_unique<TitleScreenState>());
}

void App::Update() {
    m_Root.Update();

    if (m_CurrentGameState) m_CurrentGameState->OnUpdate(*this);

    if (Util::Input::IsKeyUp(Util::Keycode::ESCAPE) || Util::Input::IfExit()) {
        m_CurrentState = State::END;
    }

    if (m_CurrentState == State::UPDATE) ValidTask();
}

void App::End() { LOG_TRACE("End"); }

void App::ValidTask() { LOG_TRACE("ValidTask"); }

void App::ShowCover()       { m_Root.AddChild(m_CoverImage); }
void App::HideCover()       { m_Root.RemoveChild(m_CoverImage); }
void App::ShowMenuBg()      { m_Root.AddChild(m_MenuBg); }
void App::HideMenuBg()      { m_Root.RemoveChild(m_MenuBg); }
void App::ShowAttackerWin() { m_Root.AddChild(m_AttackImage); }
void App::ShowDefenderWin() { m_Root.AddChild(m_DefenseImage); }
void App::HideWinScreens() {
    m_Root.RemoveChild(m_AttackImage);
    m_Root.RemoveChild(m_DefenseImage);
}

void App::ShowMenu(Menu& menu, float x, float y) { menu.Show(m_Root, x, y); }
void App::HideMenu(Menu& menu)                   { menu.Hide(m_Root); }
