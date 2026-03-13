#include "App.hpp"

#include "Util/Image.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"
#include "Util/Logger.hpp"

void App::Start() {
    LOG_TRACE("Start");

    m_CoverImage = std::make_shared<BackgroundImage>();  // 載入封面圖片
    m_Root.AddChild(m_CoverImage);  // 將封面圖片加入根節點
    // m_LevelManager.LoadLevel(RESOURCE_DIR"/Map/level_1.txt");  // 預先載入第一關

    m_Player = std::make_shared<Player>(1, 1);  // 將角色加入根節點

    m_CurrentState = State::UPDATE;
}

void App::Update() {
    m_Root.Update();  // 更新場景

    if (m_GameState == GameState::TITLE_SCREEN) {  // 如果在 TITLE_SCREEN (封面)
        // m_CoverImage->Draw();  // 繪製封面圖片 (用 Renderer 繪圖，不需要這一行)

        if (Util::Input::IsKeyPressed(Util::Keycode::SPACE)) {  // 偵測空白鍵
            LOG_INFO("Start Game");
            m_GameState = GameState::GAMEPLAY;  // 切換到 GAMEPLAY (遊戲)

            

            m_Root.RemoveChild(m_CoverImage);       // 移除封面圖片
            m_LevelManager.LoadLevel(RESOURCE_DIR"/Map/level_1.txt");  // 載入第一關
            m_LevelManager.AttachToRoot(m_Root);    // 載入地圖方塊
            
            m_Player = std::make_shared<Player>(1, 1);  // 重新建立角色
            m_Root.AddChild(m_Player);  // 將角色加入根節點
        }
    }
    else if (m_GameState == GameState::GAMEPLAY) {  // 如果在 GAMEPLAY (遊戲)
        // LOG_INFO(m_Player->IsDead());
        
        if (!m_Player->IsDead()) {  // 玩家活著才允許移動與放炸彈
            // m_Player->Update(m_LevelManager);  // 更新角色
            m_Player->Update(m_LevelManager, m_BombManager);  // 更新角色
            // LOG_INFO("Update Player");

            // 按下空白鍵放炸彈 (目前火力暫定寫死為 2)
            if (Util::Input::IsKeyDown(Util::Keycode::SPACE)) {
                LOG_INFO("Bomb");
                // m_BombManager.PlaceBomb(m_Player->GetGridX(), m_Player->GetGridY(), 2, m_Root);
                m_BombManager.PlaceBomb(m_Player, m_LevelManager, m_Root);
            }

            // 運算物理與傷害
            m_BombManager.Update(m_LevelManager, m_Root, m_Player);
        }
		else {  // 玩家死亡
            /*if (m_DeathCountdown == -1) {
                m_DeathCountdown = 30;
            }

            m_Root.RemoveChild(m_Player);  // 先移除角色

            m_DeathCountdown--;  // 開始死亡倒數

			m_BombManager.Update(m_LevelManager, m_Root, m_Player);  // 讓炸彈與火焰繼續運作，確保玩家能看到死亡動畫與傷害判定

            // 死亡倒數結束
            if (m_DeathCountdown <= 0) {
                LOG_INFO("Game Over!");

                // 從畫面上拔除所有遊戲物件
                m_LevelManager.DetachFromRoot(m_Root);
                m_BombManager.Clear(m_Root);

                m_Root.AddChild(m_CoverImage);
                m_GameState = GameState::TITLE_SCREEN;   // 暫時的死亡懲罰：踢回首頁

				m_DeathCountdown = -1;  // 重置死亡倒數計時
            }*/

            if (m_DeathCountdown == -1 && m_RespawnTimer == -1) {
                m_DeathCountdown = 30;
                LOG_INFO("Player died, showing body...");
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
