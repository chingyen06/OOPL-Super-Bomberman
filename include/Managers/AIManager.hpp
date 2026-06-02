#ifndef AIMANAGER_HPP
#define AIMANAGER_HPP

#include <vector>
#include <memory>
#include <unordered_set>
#include <utility>
#include <functional>
#include "glm/vec2.hpp"
#include "Bot/Pathfinder.hpp"
#include "DangerMap.hpp"
#include "Player.hpp"
#include "LevelManager.hpp"
#include "BombManager.hpp"
#include "InteractableManager.hpp"

class IProgrammableController;
class Spirit;
class TurretManager;

class AIManager {
public:
    void Update(std::vector<std::shared_ptr<Player>>& players,
                const LevelManager& levelManager,
                const BombManager& bombManager,
                const InteractableManager& interactableManager,
                const std::vector<std::shared_ptr<Spirit>>& spirits,
                const TurretManager& turretManager);

    // debug overlay 用：讀取上一次 Update 算出的危險地圖
    bool IsDangerAt(int x, int y) const { return m_DangerMap.IsDanger(x, y); }

private:
    // 放彈前驗證逃生路徑可行 (FindSafeSpot + 走得到)。可行則回 true、outFirstStep 設為逃生第一步，
    // 並把這顆 pending 炸彈登記進 DangerMap。三個放彈策略 (追擊/炸牆/自殺) 共用，消除重複。
    bool TryPlanBombEscape(int botX, int botY, int mapW, int mapH,
                           const LevelManager& lm, const BombManager& bm, int fp,
                           const std::function<int(int, int)>& walkCost,
                           std::pair<int, int>& outFirstStep);

    // claimed: 已被其他 bot 鎖定的目標，會被排除以避免多隻 bot 衝向同一物
    std::shared_ptr<Interactable> FindNearestTarget(int botX, int botY, bool hasKey,
                                                     const std::vector<std::shared_ptr<Interactable>>& items,
                                                     const std::unordered_set<Interactable*>& claimed) const;

    // 每幀一次的目標分派：先保留各 bot「仍有效的承諾」(穩定、不抽搐)，再把沒目標的 bot
    // 指派到最近且「尚未被佔用」的物件 → 每隻 bot 追不同目標，不會全擠同一個。
    void AssignTargets(std::vector<std::shared_ptr<Player>>& players,
                       const std::vector<std::shared_ptr<Interactable>>& items);

    // 回傳 (gridX,gridY) 上、對本 bot 仍有效 (priority>0) 的物件目標；用於「目標承諾」續追。
    std::shared_ptr<Interactable> FindTargetAt(int gridX, int gridY, bool hasKey,
                                               const std::vector<std::shared_ptr<Interactable>>& items) const;

    // 無法安全抵達目標時的「靠近並等待」：移動到安全可達、且離目標最近的格 (而非原地放棄)。
    // 有移動回 true。例如目標旁有人放炸彈，先靠過去、等火焰過了策略 3 再接手開寶箱。
    bool ApproachTarget(IProgrammableController* botController, int botX, int botY, int targetX, int targetY,
                        int mapW, int mapH, const std::function<int(int, int)>& safeCost, glm::vec2 force);

    // 送出主方向鍵；若身處輸送帶 (force≠0)，再補按一個與帶力垂直分量相反的方向，
    // 抵銷側向被帶偏移 (否則過不去轉角、原地抽搐)。一般地面 force=0 → 只送主方向。
    void ExecuteMove(IProgrammableController* botController, int fromX, int fromY, int toX, int toY,
                     bool placeBomb, glm::vec2 force) const;

    DangerMap  m_DangerMap;  // 危險地圖 (拆分自原 AIManager 內聚實作)
    Pathfinder m_Pathfinder; // 泛用 A* 尋路 (拆分自原 AIManager 內聚實作)
};

#endif