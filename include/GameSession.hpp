#ifndef GAMESESSION_HPP
#define GAMESESSION_HPP

#include <memory>
#include <vector>

#include "AIManager.hpp"
#include "BombManager.hpp"
#include "CheatManager.hpp"
#include "MatchConfig.hpp"
#include "InteractableManager.hpp"
#include "LevelManager.hpp"
#include "Player.hpp"
#include "Spirit.hpp"
#include "Turret/TurretManager.hpp"
#include "UI/UIManager.hpp"
#include "Util/Renderer.hpp"

// 一次性的對戰會話：持有所有 gameplay-side 的 manager 與 entity。
// App 不再直接觸碰這些成員 (移除 friend)；State 透過 App::Session() 取得 reference。
class GameSession {
public:
    explicit GameSession(Util::Renderer& root);

    void LoadLevel(int levelIndex);  // 載入關卡 + spawn 玩家、Spirit、砲台
    void Update();                   // 推進一個 tick (manager + entity)
    void Clear();                    // 把所有實體從場景移除 (GameEnd 用)

    // 勝負查詢
    bool IsAttackerWin() const;
    bool IsTimeUp() const { return m_GameTime == 0; }

    // 暫停選單用：作弊模式開關 + 目前關卡 (供「再次開始」重載)
    void ToggleCheat()        { m_CheatManager.Toggle(); }
    bool IsCheatEnabled() const { return m_CheatManager.IsEnabled(); }
    int  GetCurrentLevel() const { return m_CurrentLevelIndex; }

    // 「更換規則」「選擇隊伍」畫面透過此設定影響下一場對戰
    MatchConfig& Config() { return m_Config; }

private:
    Util::Renderer& m_Root;

    LevelManager        m_LevelManager;
    BombManager         m_BombManager;
    InteractableManager m_InteractableManager;
    AIManager           m_AIManager;
    TurretManager       m_TurretManager;
    UIManager           m_UIManager;
    CheatManager        m_CheatManager;

    std::vector<std::shared_ptr<Player>> m_Players;
    std::vector<std::shared_ptr<Spirit>> m_Spirits;

    Player* m_HumanPlayer = nullptr;  // 人類控制的玩家 (作弊模式作用對象)；指向 m_Players 內的物件

    MatchConfig m_Config;         // 規則 / 陣營設定
    int m_CurrentLevelIndex = 1;  // 最近載入的關卡 (「再次開始」用)
    int m_GameTime = -1;
};

#endif
