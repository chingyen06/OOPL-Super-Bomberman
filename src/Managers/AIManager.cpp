#include "AIManager.hpp"

#include <cmath>
#include <map>

#include "Controller/IProgrammableController.hpp"
#include "Interactable.hpp"
#include "Player.hpp"

void AIManager::Update(std::vector<std::shared_ptr<Player>>& players,
                       const LevelManager& levelManager,
                       const BombManager& bombManager,
                       const InteractableManager& interactableManager,
                       const std::vector<std::shared_ptr<Spirit>>& spirits,
                       const TurretManager& turretManager) {

    m_DangerMap.Rebuild(levelManager, bombManager);

    const auto& interactables = interactableManager.GetInteractables();

    // 先做目標分派：保留各 bot 仍有效的承諾，再把沒目標的指派到最近未佔用物件。
    // 之後決策迴圈只「讀取」各 bot 已分派好的目標 → 目標穩定 (不抽搐) 且彼此不同 (不浪費)。
    AssignTargets(players, interactables);

    for (auto& bot : players) {
        if (bot->IsDead()) continue;
        // Cross-cast 到 IProgrammableController：HumanController 不實作此介面所以會回 nullptr，
        // 自動被略過 — 不依賴具體 BotController 類別。
        auto botController = dynamic_cast<IProgrammableController*>(bot->GetController());
        if (!botController) continue;

        m_Decision.DecideForBot(*bot, *botController, interactables, players,
                                levelManager, bombManager, interactableManager,
                                spirits, turretManager);
    }
}

std::shared_ptr<Interactable> AIManager::FindNearestTarget(int botX, int botY, bool hasKey,
                                                            const std::vector<std::shared_ptr<Interactable>>& items,
                                                            const std::unordered_set<Interactable*>& claimed) const {
    // 優先序資料化：不再 dynamic_pointer_cast<PowerUp/Chest/Key>，改詢問 Interactable
    // 的 GetAttackerTargetPriority。每個 priority 層內取距離最近者，跨層則依 std::map
    // 排序 (priority 小 = 優先) 由低到高挑第一個非空層。
    //
    // 新增一種 bot 目標型別 = 新類別 override GetAttackerTargetPriority — 不必動 AIManager。
    std::map<int, std::pair<std::shared_ptr<Interactable>, int>> bestByPriority;

    for (const auto& item : items) {
        if (claimed.count(item.get())) continue;  // 已被其他 bot 鎖定
        const int prio = item->GetAttackerTargetPriority(hasKey);
        if (prio <= 0) continue;  // 不可選為目標 (e.g. 沒鑰匙的 bot 看到 Chest)

        const int dist = std::abs(item->GetGridX() - botX) + std::abs(item->GetGridY() - botY);
        auto it = bestByPriority.find(prio);
        if (it == bestByPriority.end() || dist < it->second.second) {
            bestByPriority[prio] = { item, dist };
        }
    }

    if (bestByPriority.empty()) return nullptr;
    return bestByPriority.begin()->second.first;  // 最低 priority 編號 = 最優先
}

void AIManager::AssignTargets(std::vector<std::shared_ptr<Player>>& players,
                              const std::vector<std::shared_ptr<Interactable>>& items) {
    std::unordered_set<Interactable*> claimed;

    // Pass 1：保留「仍有效」的承諾。
    for (auto& b : players) {
        if (b->IsDead()) continue;
        auto ctrl = dynamic_cast<IProgrammableController*>(b->GetController());
        if (!ctrl) continue;
        auto t = FindTargetAt(ctrl->GoalX(), ctrl->GoalY(), b->HasKey(), items);
        if (t && !claimed.count(t.get())) claimed.insert(t.get());
        else ctrl->SetGoal(-1, -1);
    }

    // Pass 2：沒有效承諾的 bot → 指派最近、尚未被佔用的物件。
    for (auto& b : players) {
        if (b->IsDead()) continue;
        auto ctrl = dynamic_cast<IProgrammableController*>(b->GetController());
        if (!ctrl || ctrl->GoalX() >= 0) continue;
        auto t = FindNearestTarget(b->GetGridX(), b->GetGridY(), b->HasKey(), items, claimed);
        if (t) { ctrl->SetGoal(t->GetGridX(), t->GetGridY()); claimed.insert(t.get()); }
    }
}

std::shared_ptr<Interactable> AIManager::FindTargetAt(int gridX, int gridY, bool hasKey,
                                                      const std::vector<std::shared_ptr<Interactable>>& items) const {
    if (gridX < 0 || gridY < 0) return nullptr;
    for (const auto& item : items) {
        if (item->GetGridX() == gridX && item->GetGridY() == gridY &&
            item->GetAttackerTargetPriority(hasKey) > 0) {
            return item;
        }
    }
    return nullptr;
}
