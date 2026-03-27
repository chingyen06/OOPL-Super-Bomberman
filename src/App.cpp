#include "App.hpp"

#include "Util/Image.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"
#include "Util/Logger.hpp"

void App::Start() {
    LOG_TRACE("Start");

    m_CoverImage = std::make_shared<BackgroundImage>(RESOURCE_DIR"/Image/cover.jpg");  // 載入封面圖片
    m_DefenseImage = std::make_shared<BackgroundImage>(RESOURCE_DIR"/Image/defense_win.png");  // 載入防守方獲勝圖片
    m_AttackImage = std::make_shared<BackgroundImage>(RESOURCE_DIR"/Image/attack_win.png");  // 載入防守方獲勝圖片
    m_Root.AddChild(m_CoverImage);  // 將封面圖片加入根節點
    // m_LevelManager.LoadLevel(RESOURCE_DIR"/Map/level_1.txt");  // 預先載入第一關

    m_CurrentState = State::UPDATE;
}

void App::LoadLevel(int levelIndex) {
    LOG_INFO("Loading Level " + std::to_string(levelIndex) + "...");

    std::string levelPath = RESOURCE_DIR"/Map/level_" + std::to_string(levelIndex) + ".txt";

    m_LevelManager.LoadLevel(levelPath, m_InteractableManager);  // 載入關卡
    m_LevelManager.AttachToRoot(m_Root);    // 載入地圖方塊
    m_InteractableManager.AttachToRoot(m_Root);  // 載入互動物件

    Control ctrl1P = {
        Util::Keycode::W, 
        Util::Keycode::S,
        Util::Keycode::A, 
        Util::Keycode::D,
        Util::Keycode::SPACE
    };

    Control ctrl2P = {
        Util::Keycode::UP,
        Util::Keycode::DOWN,
        Util::Keycode::LEFT,
        Util::Keycode::RIGHT,
        Util::Keycode::KP_ENTER
    };

    auto p1 = std::make_shared<Player>(1, 1, Team::DEFENDER, ctrl1P);
    m_Players.push_back(p1);
    m_Root.AddChild(p1);

    auto p2 = std::make_shared<Player>(23, 15, Team::ATTACKER, ctrl2P);
    m_Players.push_back(p2);
    m_Root.AddChild(p2);

    m_GameTime = 60 * 60 * 3;  // 遊戲時間 (3 分鐘)

    m_UIManager.Init(m_Root);
}

void App::Update() {
    m_Root.Update();  // 更新場景

    if (m_GameState == GameState::TITLE_SCREEN) {  // 如果在 TITLE_SCREEN (封面)
        // m_CoverImage->Draw();  // 繪製封面圖片 (用 Renderer 繪圖，不需要這一行)

        if (Util::Input::IsKeyUp(Util::Keycode::SPACE)) {  // 偵測空白鍵
            LOG_INFO("Start Game");
            m_GameState = GameState::GAMEPLAY;

            m_Root.RemoveChild(m_CoverImage);
            
            LoadLevel(1);
        }
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
        m_UIManager.Update(m_GameTime, m_Players);

        for (auto& player : m_Players) {
            player->Update(m_LevelManager, m_BombManager, m_InteractableManager);

            if (!player->IsDead()) {
                if (Util::Input::IsKeyDown(player->GetBombKey())) {
                    m_BombManager.PlaceBomb(player, m_LevelManager, m_InteractableManager, m_Root);
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
    }
    else if (m_GameState == GameState::GAMEEND) {
        // 清除戰場
        m_InteractableManager.Clear(m_Root);
        m_BombManager.Clear(m_Root);
        m_LevelManager.DetachFromRoot(m_Root);
        m_UIManager.Clear(m_Root);

        for (auto& player : m_Players)
            m_Root.RemoveChild(player);

        if (Util::Input::IsKeyUp(Util::Keycode::SPACE)) {  // 偵測空白鍵
            LOG_INFO("Return to Title Screen");

            m_Root.RemoveChild(m_AttackImage);
            m_Root.RemoveChild(m_DefenseImage);
            m_UIManager.Clear(m_Root);

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