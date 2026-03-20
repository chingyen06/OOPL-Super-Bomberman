#ifndef APP_HPP
#define APP_HPP

#include "pch.hpp" // IWYU pragma: export
#include <memory>

#include "Util/Renderer.hpp"
#include "BackgroundImage.hpp"
#include "LevelManager.hpp"
#include "Player.hpp"
#include "BombManager.hpp"
#include "InteractableManager.hpp"

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

	void LoadLevel(int levelIndex);

    void End(); // NOLINT(readability-convert-member-functions-to-static)

private:
    void ValidTask();

private:
    enum class GameState {
        TITLE_SCREEN,
        GAMEPLAY,
        GAMEEND
    };

    State m_CurrentState = State::START;
    GameState m_GameState = GameState::TITLE_SCREEN;  // 初始為 TITLE_SCREEN (封面)

    Util::Renderer m_Root;  // 場景的根節點

    std::shared_ptr<BackgroundImage> m_CoverImage;  // 存封面圖片的 pointer
	std::shared_ptr<BackgroundImage> m_DefenseImage;  // 存防守方獲勝圖片的 pointer
    std::shared_ptr<BackgroundImage> m_AttackImage;  // 存進攻方獲勝圖片的 pointer
    LevelManager m_LevelManager;  // 管理關卡

    std::shared_ptr<Player> m_Player;  // 存角色的 pointer

	BombManager m_BombManager;  // 管理炸彈與火焰

	int m_DeathCountdown = -1;  // 死亡倒數計時
	int m_RespawnTimer = -1;  // 重生倒數計時

	InteractableManager m_InteractableManager;  // 管理互動物件

	int m_GameTime = -1;  // 遊戲時間
};

#endif
