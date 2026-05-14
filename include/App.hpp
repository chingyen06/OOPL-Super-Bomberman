#ifndef APP_HPP
#define APP_HPP

#include "pch.hpp" // IWYU pragma: export
#include <memory>

#include "Util/Renderer.hpp"
#include "UI/UIImage.hpp"
#include "LevelManager.hpp"
#include "Player.hpp"
#include "BombManager.hpp"
#include "InteractableManager.hpp"
#include "UI/UIManager.hpp"
#include "AIManager.hpp"
#include "Menu.hpp"
#include "Spirit.hpp"

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
        LEVEL_SELECT,
        GAMEPLAY,
        GAMEEND
    };

    State m_CurrentState = State::START;
    GameState m_GameState = GameState::TITLE_SCREEN;

    Util::Renderer m_Root;  // 場景的根節點

	// 背景圖片
    std::shared_ptr<UIImage> m_CoverImage;
	std::shared_ptr<UIImage> m_DefenseImage;
    std::shared_ptr<UIImage> m_AttackImage;

    LevelManager m_LevelManager;  // 管理關卡

    // std::shared_ptr<Player> m_Player;  // 角色
    std::vector<std::shared_ptr<Player>> m_Players;

	BombManager m_BombManager;  // 管理炸彈與火焰

	int m_DeathCountdown = -1;  // 死亡倒數計時
	int m_RespawnTimer = -1;  // 重生倒數計時

	InteractableManager m_InteractableManager;  // 管理互動物件

	int m_GameTime = -1;  // 遊戲時間

	UIManager m_UIManager;  // 管理 UI 顯示

    AIManager m_AIManager;

    std::vector<std::shared_ptr<Spirit>> m_Spirits;

    // 選單
    Menu m_MainMenu;
    Menu m_LevelMenu;
    std::shared_ptr<UIImage> m_MenuBg;
};

#endif
