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

    m_Player = std::make_shared<Player>(1, 1);  // 將角色加入根節點

    m_CurrentState = State::UPDATE;
}

void App::LoadLevel(int levelIndex) {
    LOG_INFO("Loading Level " + std::to_string(levelIndex) + "...");

    std::string levelPath = RESOURCE_DIR"/Map/level_" + std::to_string(levelIndex) + ".txt";

    m_LevelManager.LoadLevel(levelPath, m_InteractableManager);  // 載入關卡
    m_LevelManager.AttachToRoot(m_Root);    // 載入地圖方塊
    m_InteractableManager.AttachToRoot(m_Root);  // 載入互動物件

    m_Player = std::make_shared<Player>(1, 1);  // 重新建立角色
    m_Root.AddChild(m_Player);  // 將角色加入根節點

    m_GameTime = 60 * 60 * 3;  // 遊戲時間 (3 分鐘)
}

void App::Update() {
    m_Root.Update();  // 更新場景

    if (m_GameState == GameState::TITLE_SCREEN) {  // 如果在 TITLE_SCREEN (封面)
        // m_CoverImage->Draw();  // 繪製封面圖片 (用 Renderer 繪圖，不需要這一行)

        if (Util::Input::IsKeyPressed(Util::Keycode::SPACE)) {  // 偵測空白鍵
            LOG_INFO("Start Game");
            m_GameState = GameState::GAMEPLAY;  // 切換到 GAMEPLAY (遊戲)

            m_Root.RemoveChild(m_CoverImage);       // 移除封面圖片
            
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

            // 清除戰場
            m_InteractableManager.Clear(m_Root);
            m_BombManager.Clear(m_Root);
            m_LevelManager.DetachFromRoot(m_Root);
			m_InteractableManager.Clear(m_Root);
            m_Root.RemoveChild(m_Player);

            m_Root.AddChild(m_AttackImage); // 進攻方勝利
            return;
        }

        if (m_GameTime == 0) {
            LOG_INFO("Defender Wins!");
            m_GameState = GameState::GAMEEND;

            // 清除戰場
            m_InteractableManager.Clear(m_Root);
            m_BombManager.Clear(m_Root);
            m_LevelManager.DetachFromRoot(m_Root);
            m_InteractableManager.Clear(m_Root);
            m_Root.RemoveChild(m_Player);

            m_Root.AddChild(m_DefenseImage); // 防守方勝利
            return;
        }
        
        m_BombManager.Update(m_LevelManager, m_InteractableManager, m_Root, m_Player);  // 運算物理與傷害
        m_InteractableManager.Update(m_Player, m_Root);  // 更新互動物件

        if (!m_Player->IsDead()) {  // 玩家活著才允許移動與放炸彈
            // m_Player->Update(m_LevelManager);  // 更新角色
            m_Player->Update(m_LevelManager, m_BombManager, m_InteractableManager);  // 更新角色
            // LOG_INFO("Update Player");

            // 按下空白鍵放炸彈
            if (Util::Input::IsKeyDown(Util::Keycode::SPACE)) {
                // LOG_INFO("Bomb");
                // m_BombManager.PlaceBomb(m_Player->GetGridX(), m_Player->GetGridY(), 2, m_Root);
                m_BombManager.PlaceBomb(m_Player, m_LevelManager, m_InteractableManager, m_Root);
            }

        }
		else {  // 玩家死亡
            if (m_DeathCountdown == -1 && m_RespawnTimer == -1) {
                m_DeathCountdown = 30;
                LOG_INFO("Player died");

                if (m_Player->HasKey()) {
					LOG_INFO("Player dropped the Key!");

                    m_Player->SetKey(false);

                    m_InteractableManager.AddKey(m_Player->GetGridX(), m_Player->GetGridY());
                    m_InteractableManager.AttachToRoot(m_Root);
                }
            }

            if (m_DeathCountdown > 0) {
                m_DeathCountdown--;
                if (m_DeathCountdown == 0) {
                    m_Root.RemoveChild(m_Player);
                    m_RespawnTimer = 90; // 等待 1.5 秒重生
                    m_DeathCountdown = -1;
                }
            }
            // 玩家消失在畫面上，但炸彈還在爆
            else if (m_RespawnTimer > 0) {
                m_RespawnTimer--;
                if (m_RespawnTimer == 0) {
					// 復活角色
                    m_Player->Respawn(1, 1); // 回到起點
                    m_Root.AddChild(m_Player);
                    m_RespawnTimer = -1;
                    LOG_INFO("Player Respawned!");
                }
            }
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