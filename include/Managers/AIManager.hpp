#ifndef AIMANAGER_HPP
#define AIMANAGER_HPP

#include <vector>
#include <memory>
#include <unordered_set>
#include <utility>
#include <functional>
#include "glm/vec2.hpp"
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
    // 泛用型 A*；尋路規則透過 cost function 注入
    std::vector<std::pair<int, int>> FindPath(int startX, int startY, int targetX, int targetY, int mapW, int mapH, std::function<int(int, int)> costFunc);

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

    // 回傳 (gridX,gridY) 上、對本 bot 仍有效 (priority>0) 的物件目標；用於「目標承諾」續追。
    std::shared_ptr<Interactable> FindTargetAt(int gridX, int gridY, bool hasKey,
                                               const std::vector<std::shared_ptr<Interactable>>& items) const;

    // 只送出主方向鍵；垂直/水平的格中心對齊交由 Player 的自動歸位 (clamp、不過衝) 負責，
    // 避免在這裡做未夾住的反向校正造成中線抽搐。
    void ExecuteMove(IProgrammableController* botController, int fromX, int fromY, int toX, int toY, bool placeBomb) const;

    DangerMap m_DangerMap;  // 危險地圖 (拆分自原 AIManager 內聚實作)
};

#endif