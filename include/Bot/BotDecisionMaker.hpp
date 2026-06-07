#ifndef BOT_DECISION_MAKER_HPP
#define BOT_DECISION_MAKER_HPP

#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include "glm/vec2.hpp"

class BombManager;
class DangerMap;
class IProgrammableController;
class Interactable;
class InteractableManager;
class LevelManager;
class Pathfinder;
class Player;
class Spirit;
class TurretManager;

// 把「單隻 bot 在一幀內如何決策」從 AIManager 抽出 (SRP)。
// AIManager 負責 (危險地圖 Rebuild、目標分派、外層 for 迴圈)；
// 本類別負責每隻 bot 的策略樹 (危險逃跑 → 安全追物件 → 衝目標 → 炸牆 → 自殺突擊 → 靠近等待)
// 以及為這些策略服務的小工具 (放彈逃生驗證、執行移動)。
//
// 不擁有 Pathfinder / DangerMap — 它們由 AIManager 持有並以 reference 傳入；本類別只在
// 該幀內讀寫 (RegisterPendingBomb 是讀寫 DangerMap)，狀態不跨幀。
class BotDecisionMaker {
public:
    BotDecisionMaker(const Pathfinder& pathfinder, DangerMap& dangerMap);

    // 為一隻活著的 bot 做完整一幀的決策 (含 TickCooldown 後的 IsReadyToDecide 檢查、
    // 反應冷卻重設、SetInput / 放彈)。AIManager 的外層迴圈只需呼叫此一函式。
    void DecideForBot(Player& bot,
                      IProgrammableController& controller,
                      const std::vector<std::shared_ptr<Interactable>>& interactables,
                      const std::vector<std::shared_ptr<Player>>& players,
                      const LevelManager& levelManager,
                      const BombManager& bombManager,
                      const InteractableManager& interactableManager,
                      const std::vector<std::shared_ptr<Spirit>>& spirits,
                      const TurretManager& turretManager);

private:
    // 放彈前驗證逃生路徑可行 (FindSafeSpot + 走得到)。可行則回 true、outFirstStep 設為逃生第一步，
    // 並把這顆 pending 炸彈登記進 DangerMap。三個放彈策略 (追擊/炸牆/自殺) 共用，消除重複。
    bool TryPlanBombEscape(int botX, int botY, int mapW, int mapH,
                           const LevelManager& lm, const BombManager& bm, int fp,
                           const std::function<int(int, int)>& walkCost,
                           std::pair<int, int>& outFirstStep) const;

    // 無法安全抵達目標時的「靠近並等待」：移動到安全可達、且離目標最近的格 (而非原地放棄)。
    // 有移動回 true。
    bool ApproachTarget(IProgrammableController& controller, int botX, int botY,
                        int targetX, int targetY, int mapW, int mapH,
                        const std::function<int(int, int)>& safeCost, glm::vec2 force) const;

    // 送出主方向鍵；若身處輸送帶 (force≠0)，再補按一個與帶力垂直分量相反的方向，
    // 抵銷側向被帶偏移 (否則過不去轉角、原地抽搐)。
    void ExecuteMove(IProgrammableController& controller, int fromX, int fromY,
                     int toX, int toY, bool placeBomb, glm::vec2 force) const;

    // 回傳 (gridX,gridY) 上、對本 bot 仍有效 (priority>0) 的物件目標；用於「目標承諾」續追。
    std::shared_ptr<Interactable> FindTargetAt(int gridX, int gridY, bool hasKey,
                                               const std::vector<std::shared_ptr<Interactable>>& items) const;

    const Pathfinder& m_Pathfinder;
    DangerMap&        m_DangerMap;
};

#endif
