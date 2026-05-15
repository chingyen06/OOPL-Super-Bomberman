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
    m_Root.AddChild(m_CoverImage);  // 將封面圖片加入根節點
    // m_LevelManager.LoadLevel(RESOURCE_DIR"/Map/level_1.txt");  // 預先載入第一關

    // 選單
    m_MainMenu.AddOption("Start Game", [this]() {
        m_MainMenu.Hide(m_Root);
        m_LevelMenu.Show(m_Root, 0, 50); // 切換到關卡選單
    });
    m_MainMenu.AddOption("Exit Game", [this]() {
        m_CurrentState = State::END;
    });

    m_LevelMenu.AddOption("Level 1", [this]() {
        m_LevelMenu.Hide(m_Root);
        m_Root.RemoveChild(m_MenuBg);
        m_GameState = GameState::GAMEPLAY;
        LoadLevel(1);
    });
    m_LevelMenu.AddOption("Level 2", [this]() {
        m_LevelMenu.Hide(m_Root);
        m_Root.RemoveChild(m_MenuBg);
        m_GameState = GameState::GAMEPLAY;
        LoadLevel(2);
    });
    m_LevelMenu.AddOption("Level 3", [this]() {
        m_LevelMenu.Hide(m_Root);
        m_Root.RemoveChild(m_MenuBg);
        m_GameState = GameState::GAMEPLAY;
        LoadLevel(3);
    });
    m_LevelMenu.AddOption("return", [this]() {
        m_LevelMenu.Hide(m_Root);
        m_MainMenu.Show(m_Root, 0, 50);
    });
    //

    m_CurrentState = State::UPDATE;
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
    if (m_GameState == GameState::TITLE_SCREEN) {  // 如果在 TITLE_SCREEN (封面)
        if (Util::Input::IsKeyUp(Util::Keycode::SPACE)) {  // 偵測空白鍵
            LOG_INFO("Enter Level Select");
            m_GameState = GameState::LEVEL_SELECT;

            // 進入選單，換掉封面圖，顯示第一層主選單
            m_Root.RemoveChild(m_CoverImage);
            m_Root.AddChild(m_MenuBg);
            m_MainMenu.Show(m_Root, 0, 50);
        }
    }
	else if (m_GameState == GameState::LEVEL_SELECT) {  // 如果在 LEVEL_SELECT (選單)
        if (m_MainMenu.IsVisible()) 
            m_MainMenu.Update();
        else if (m_LevelMenu.IsVisible()) 
            m_LevelMenu.Update();
    }
    else if (m_GameState == GameState::GAMEPLAY) {  // 如果在 GAMEPLAY (遊戲)
        if (m_GameTime > 0) 
            m_GameTime--;

        if (m_GameTime % 60 == 0) {
            int seconds = m_GameTime / 60;
            LOG_INFO("Time Remaining: " + std::to_string(seconds) + "s");
		}

        if (m_InteractableManager.IsAllChestOpened()) {
            LOG_INFO("Attacker Wins!");
            m_GameState = GameState::GAMEEND;

            m_Root.AddChild(m_AttackImage); // 進攻方勝利
            return;
        }

        if (m_GameTime == 0) {
            LOG_INFO("Defender Wins!");
            m_GameState = GameState::GAMEEND;

            m_Root.AddChild(m_DefenseImage); // 防守方勝利
            return;
        }

        m_BombManager.Update(m_LevelManager, m_InteractableManager, m_Root, m_Players);
        m_InteractableManager.Update(m_Players, m_Root);
        auto statusList = m_InteractableManager.GetChestStatusList();
        m_UIManager.Update(m_GameTime, m_Players, statusList, m_Root);
        m_AIManager.Update(m_Players, m_LevelManager, m_BombManager, m_InteractableManager);
        m_TurretManager.Update(m_Players, m_LevelManager, m_BombManager, m_InteractableManager, m_Root);

        GameWorldContext worldContext(m_LevelManager, m_BombManager, m_InteractableManager, m_TurretManager);

        for (auto& player : m_Players) {
            player->Update(worldContext);

            if (!player->IsDead()) {
                if (player->GetController() && player->GetController()->IsPlaceBombPressed()) {
                    m_BombManager.PlaceBomb(player, m_LevelManager, m_InteractableManager, m_Root, m_Players);
                }
            }
            else {
                if (player->HasKey()) {
                    LOG_INFO("Player dropped the Key!");
                    player->SetKey(false);
                    m_InteractableManager.DropKey(player->GetGridX(), player->GetGridY(), m_Root);
                }
            }
        }

        for (auto it = m_Spirits.begin(); it != m_Spirits.end();) {
            auto& spirit = *it;

            spirit->Update(m_Players, m_LevelManager, m_BombManager);

            if (spirit->ShouldDelete()) {
                m_Root.RemoveChild(spirit);
                it = m_Spirits.erase(it);
            }
            else {
                ++it;
            }
        }
    }
    else if (m_GameState == GameState::GAMEEND) {
        // 清除戰場
        m_InteractableManager.Clear(m_Root);
        m_BombManager.Clear(m_Root);
        m_LevelManager.DetachFromRoot(m_Root);
        m_UIManager.Clear(m_Root);
        m_TurretManager.Clear(m_Root);

        for (auto& s : m_Spirits) 
            m_Root.RemoveChild(s);
        m_Spirits.clear();

        for (auto& player : m_Players)
            m_Root.RemoveChild(player);
        m_Players.clear();

        if (Util::Input::IsKeyUp(Util::Keycode::SPACE)) {  // 偵測空白鍵
            LOG_INFO("Return to Title Screen");

            m_Root.RemoveChild(m_AttackImage);
            m_Root.RemoveChild(m_DefenseImage);

            m_Root.AddChild(m_CoverImage);  // 將封面圖片加入根節點

            m_GameState = GameState::TITLE_SCREEN;
        }
    }
    
    /*
     * Do not touch the code below as they serve the purpose for
     * closing the window.
     */
    if (Util::Input::IsKeyUp(Util::Keycode::ESCAPE) ||
        Util::Input::IfExit()) {
        m_CurrentState = State::END;
    }
}

void App::End() { // NOLINT(this method will mutate members in the future)
    LOG_TRACE("End");
}


void App::ValidTask() {
}