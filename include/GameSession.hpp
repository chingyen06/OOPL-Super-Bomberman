#ifndef GAMESESSION_HPP
#define GAMESESSION_HPP

#include <memory>
#include <vector>

#include "AIManager.hpp"
#include "BombManager.hpp"
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

private:
    Util::Renderer& m_Root;

    LevelManager        m_LevelManager;
    BombManager         m_BombManager;
    InteractableManager m_InteractableManager;
    AIManager           m_AIManager;
    TurretManager       m_TurretManager;
    UIManager           m_UIManager;

    std::vector<std::shared_ptr<Player>> m_Players;
    std::vector<std::shared_ptr<Spirit>> m_Spirits;

    int m_GameTime = -1;
};

#endif
