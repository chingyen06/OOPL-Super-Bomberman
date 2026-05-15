#include "App.hpp"
#include "Controller/HumanController.hpp"
#include "Controller/BotController.hpp"
#include <algorithm>
#include <random>
#include <chrono>
#include "Util/Image.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"
#include "Util/Logger.hpp"
#include "GameWorldContext.hpp"

class TitleScreenState : public IGameState {
public:
    void OnEnter(App& app) override;
    void OnUpdate(App& app) override;
};

class LevelSelectState : public IGameState {
public:
    void OnEnter(App& app) override;
    void OnUpdate(App& app) override;
    void OnExit(App& app) override;
};

class GameplayState : public IGameState {
public:
    void OnUpdate(App& app) override;
};

class GameEndState : public IGameState {
public:
    void OnEnter(App& app) override;
    void OnUpdate(App& app) override;
};

void TitleScreenState::OnEnter(App& app) {
    app.m_Root.AddChild(app.m_CoverImage);
}
void TitleScreenState::OnUpdate(App& app) {
    if (Util::Input::IsKeyUp(Util::Keycode::SPACE)) {
        LOG_INFO("Enter Level Select");
        app.m_Root.RemoveChild(app.m_CoverImage);
        app.TransitionTo(std::make_unique<LevelSelectState>());
    }
}

void LevelSelectState::OnEnter(App& app) {
    app.m_Root.AddChild(app.m_MenuBg);
    app.m_MainMenu.Show(app.m_Root, 0, 50);
}
void LevelSelectState::OnUpdate(App& app) {
    if (app.m_MainMenu.IsVisible()) 
        app.m_MainMenu.Update();
    else if (app.m_LevelMenu.IsVisible()) 
        app.m_LevelMenu.Update();
}
void LevelSelectState::OnExit(App& app) {
    app.m_Root.RemoveChild(app.m_MenuBg);
    if (app.m_MainMenu.IsVisible()) app.m_MainMenu.Hide(app.m_Root);
    if (app.m_LevelMenu.IsVisible()) app.m_LevelMenu.Hide(app.m_Root);
}

void GameplayState::OnUpdate(App& app) {
    if (app.m_GameTime > 0) 
        app.m_GameTime--;

    if (app.m_GameTime % 60 == 0) {
        int seconds = app.m_GameTime / 60;
        LOG_INFO("Time Remaining: " + std::to_string(seconds) + "s");
    }

    if (app.m_InteractableManager.IsAllChestOpened()) {
        LOG_INFO("Attacker Wins!");
        app.m_Root.AddChild(app.m_AttackImage);
        app.TransitionTo(std::make_unique<GameEndState>());
        return;
    }

    if (app.m_GameTime == 0) {
        LOG_INFO("Defender Wins!");
        app.m_Root.AddChild(app.m_DefenseImage);
        app.TransitionTo(std::make_unique<GameEndState>());
        return;
    }

    app.m_BombManager.Update(app.m_LevelManager, app.m_InteractableManager, app.m_Root, app.m_Players);
    app.m_InteractableManager.Update(app.m_Players, app.m_Root);
    auto statusList = app.m_InteractableManager.GetChestStatusList();
    app.m_UIManager.Update(app.m_GameTime, app.m_Players, statusList, app.m_Root);
    app.m_AIManager.Update(app.m_Players, app.m_LevelManager, app.m_BombManager, app.m_InteractableManager);
    app.m_TurretManager.Update(app.m_Players, app.m_LevelManager, app.m_BombManager, app.m_InteractableManager, app.m_Root);

    GameWorldContext worldContext(app.m_LevelManager, app.m_BombManager, app.m_InteractableManager, app.m_TurretManager);

    for (auto& player : app.m_Players) {
        player->Update(worldContext);

        if (!player->IsDead()) {
            if (player->GetController() && player->GetController()->IsPlaceBombPressed()) {
                app.m_BombManager.PlaceBomb(player, app.m_LevelManager, app.m_InteractableManager, app.m_Root, app.m_Players);
            }
        }
        else {
            if (player->HasKey()) {
                LOG_INFO("Player dropped the Key!");
                player->SetKey(false);
                app.m_InteractableManager.DropKey(player->GetGridX(), player->GetGridY(), app.m_Root);
            }
        }
    }

    for (auto it = app.m_Spirits.begin(); it != app.m_Spirits.end();) {
        auto& spirit = *it;

        spirit->Update(app.m_Players, app.m_LevelManager, app.m_BombManager);

        if (spirit->ShouldDelete()) {
            app.m_Root.RemoveChild(spirit);
            it = app.m_Spirits.erase(it);
        }
        else {
            ++it;
        }
    }
}

void GameEndState::OnEnter(App& app) {
    app.m_InteractableManager.Clear(app.m_Root);
    app.m_BombManager.Clear(app.m_Root);
    app.m_LevelManager.DetachFromRoot(app.m_Root);
    app.m_UIManager.Clear(app.m_Root);
    app.m_TurretManager.Clear(app.m_Root);

    for (auto& s : app.m_Spirits) 
        app.m_Root.RemoveChild(s);
    app.m_Spirits.clear();

    for (auto& player : app.m_Players)
        app.m_Root.RemoveChild(player);
    app.m_Players.clear();
}

void GameEndState::OnUpdate(App& app) {
    if (Util::Input::IsKeyUp(Util::Keycode::SPACE)) {
        LOG_INFO("Return to Title Screen");
        app.m_Root.RemoveChild(app.m_AttackImage);
        app.m_Root.RemoveChild(app.m_DefenseImage);
        app.TransitionTo(std::make_unique<TitleScreenState>());
    }
}

void App::TransitionTo(std::unique_ptr<IGameState> next) {
    if (m_CurrentGameState) {
        m_CurrentGameState->OnExit(*this);
    }
    m_CurrentGameState = std::move(next);
    if (m_CurrentGameState) {
        m_CurrentGameState->OnEnter(*this);
    }
}

void App::Start() {
    LOG_TRACE("Start");

    m_CoverImage = std::make_shared<UIImage>(RESOURCE_DIR"/Image/cover.jpg");  // 載入封面圖片
    m_CoverImage->SetFullScreen();
    m_DefenseImage = std::make_shared<UIImage>(RESOURCE_DIR"/Image/defense_win.png");  // 載入防守方獲勝圖片
    m_DefenseImage->SetFullScreen();
    m_AttackImage = std::make_shared<UIImage>(RESOURCE_DIR"/Image/attack_win.png");  // 載入進攻方獲勝圖片
    m_AttackImage->SetFullScreen();
	m_MenuBg = std::make_shared<UIImage>(RESOURCE_DIR"/Image/white.png");  // 選單背景圖
    m_MenuBg->SetFullScreen();
    
    m_MainMenu.AddOption("Start Game", [this]() {
        TransitionTo(std::make_unique<LevelSelectState>());
        m_LevelMenu.Show(m_Root, 0, 50); // 切換到關卡選單
    });
    m_MainMenu.AddOption("Exit Game", [this]() {
        m_CurrentState = State::END;
    });

    m_LevelMenu.AddOption("Level 1", [this]() {
        TransitionTo(std::make_unique<GameplayState>());
        LoadLevel(1);
    });
    m_LevelMenu.AddOption("Level 2", [this]() {
        TransitionTo(std::make_unique<GameplayState>());
        LoadLevel(2);
    });
    m_LevelMenu.AddOption("Level 3", [this]() {
        TransitionTo(std::make_unique<GameplayState>());
        LoadLevel(3);
    });
    m_LevelMenu.AddOption("return", [this]() {
        TransitionTo(std::make_unique<LevelSelectState>());
    });

    m_CurrentState = State::UPDATE;
    TransitionTo(std::make_unique<TitleScreenState>());
}

void App::LoadLevel(int levelIndex) {
    LOG_INFO("Loading Level " + std::to_string(levelIndex) + "...");

    std::string levelPath = RESOURCE_DIR"/Map/level_" + std::to_string(levelIndex) + ".txt";

    m_LevelManager.Clear(m_Root);
    m_InteractableManager.Clear(m_Root);
    m_BombManager.Clear(m_Root);
    m_UIManager.Clear(m_Root);

    m_LevelManager.LoadLevel(levelPath, m_InteractableManager);  // 載入關卡
    m_LevelManager.AttachToRoot(m_Root);    // 載入地圖方塊
    m_InteractableManager.AttachToRoot(m_Root);  // 載入互動物件
    int totalChests = m_InteractableManager.GetTotalChestCount();  // 取得寶箱總數
    m_UIManager.Init(m_Root, totalChests);

    // 出生點
    auto defSpawn = m_LevelManager.GetDefenderSpawn();
    auto atkSpawns = m_LevelManager.GetAttackerSpawns();
    // 隨機洗牌
    static std::random_device rd;
    static std::mt19937 g(rd());   // Mersenne Twister
    std::shuffle(atkSpawns.begin(), atkSpawns.end(), g);

    auto ctrl1P = std::make_unique<HumanController>(Control{
        Util::Keycode::W, 
        Util::Keycode::S,
        Util::Keycode::A, 
        Util::Keycode::D,
        Util::Keycode::SPACE
    });

    auto ctrl2P = std::make_unique<BotController>();

    auto p1 = std::make_shared<Player>(defSpawn.first, defSpawn.second, Team::DEFENDER, std::move(ctrl1P), 0);
    m_Players.push_back(p1);
    m_Root.AddChild(p1);

    auto p2 = std::make_shared<Player>(atkSpawns[0].first, atkSpawns[0].second, Team::ATTACKER, std::move(ctrl2P), 1);
    m_Players.push_back(p2);
    m_Root.AddChild(p2);

    // 源石精靈
    m_Spirits.clear();
    for (const auto& sp : m_LevelManager.GetSpiritSpawns()) {
        auto spirit = std::make_shared<Spirit>(sp.first, sp.second);
        m_Spirits.push_back(spirit);
        m_Root.AddChild(spirit);
    }

    // 砲台
    m_TurretManager.Clear(m_Root);
    for (const auto& sp : m_LevelManager.GetTurretSpawns()) {
        m_TurretManager.AddTurret(std::make_shared<RotatingTurret>(sp.first, sp.second, Player::Direction::DOWN), m_Root);
    }

    m_GameTime = 60 * 60 * 3;  // 遊戲時間 (3 分鐘)
}

void App::Update() {
    m_Root.Update();  // 更新場景
    
    if (m_CurrentGameState) {
        m_CurrentGameState->OnUpdate(*this);
    }

    // 當前按鈕按下會呼叫該按鈕的 callback
    if (Util::Input::IsKeyUp(Util::Keycode::ESCAPE) || Util::Input::IfExit()) {
        m_CurrentState = State::END;
    }

    if (m_CurrentState == State::UPDATE) {
        ValidTask();
    }
}

void App::End() {
    LOG_TRACE("End");
}

void App::ValidTask() {
    LOG_TRACE("ValidTask");
}