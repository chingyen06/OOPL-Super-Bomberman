#ifndef AIMANAGER_HPP
#define AIMANAGER_HPP

#include <vector>
#include <memory>
#include <unordered_set>

#include "Bot/BotDecisionMaker.hpp"
#include "Bot/DangerMap.hpp"
#include "Bot/Pathfinder.hpp"
#include "Player.hpp"
#include "LevelManager.hpp"
#include "BombManager.hpp"
#include "InteractableManager.hpp"

class Spirit;
class TurretManager;

// 進攻方 bot 的協調者：每幀 (1) Rebuild 危險地圖 (2) 分派目標 (避免多 bot 互搶)
// (3) 委派 BotDecisionMaker 為每隻 bot 做策略決定。原本 (3) 的策略邏輯與工具方法已抽出
// 到 BotDecisionMaker (SRP)；本類別只保留協調 + 目標分派職責。
class AIManager {
public:
    AIManager() : m_Decision(m_Pathfinder, m_DangerMap) {}

    void Update(std::vector<std::shared_ptr<Player>>& players,
                const LevelManager& levelManager,
                const BombManager& bombManager,
                const InteractableManager& interactableManager,
                const std::vector<std::shared_ptr<Spirit>>& spirits,
                const TurretManager& turretManager);

    // debug overlay 用：讀取上一次 Update 算出的危險地圖
    bool IsDangerAt(int x, int y) const { return m_DangerMap.IsDanger(x, y); }

private:
    // 每幀一次的目標分派：先保留各 bot「仍有效的承諾」(穩定、不抽搐)，再把沒目標的 bot
    // 指派到最近且「尚未被佔用」的物件 → 每隻 bot 追不同目標，不會全擠同一個。
    void AssignTargets(std::vector<std::shared_ptr<Player>>& players,
                       const std::vector<std::shared_ptr<Interactable>>& items);

    // claimed: 已被其他 bot 鎖定的目標，會被排除以避免多隻 bot 衝向同一物
    std::shared_ptr<Interactable> FindNearestTarget(int botX, int botY, bool hasKey,
                                                     const std::vector<std::shared_ptr<Interactable>>& items,
                                                     const std::unordered_set<Interactable*>& claimed) const;

    // 同 BotDecisionMaker::FindTargetAt — AssignTargets 也需要查 (gridX,gridY) 上是否還有有效目標
    std::shared_ptr<Interactable> FindTargetAt(int gridX, int gridY, bool hasKey,
                                               const std::vector<std::shared_ptr<Interactable>>& items) const;

    DangerMap        m_DangerMap;   // 危險地圖
    Pathfinder       m_Pathfinder;  // 泛用 A* 尋路
    BotDecisionMaker m_Decision;    // 單隻 bot 的策略樹 (抽出，SRP)
};

#endif
