#include "AIManager.hpp"
#include "GameConstants.hpp"
#include "GameTypes.hpp"
#include "GridCoord.hpp"
#include "Interactable.hpp"
#include "Player.hpp"
#include "Spirit.hpp"
#include "Turret/TurretManager.hpp"
#include "Bot/BotNavigator.hpp"
#include "Controller/IProgrammableController.hpp"
#include <queue>
#include <cmath>
#include <algorithm>
#include <functional>
#include <map>

struct AStarNode {
    int x, y;
    int g, h;
    std::shared_ptr<AStarNode> parent;
    int f() const { return g + h; }
};

struct CompareNode {
    bool operator()(const std::shared_ptr<AStarNode>& a, const std::shared_ptr<AStarNode>& b) {
        return a->f() > b->f();
    }
};

std::vector<std::pair<int, int>> AIManager::FindPath(int startX, int startY, int targetX, int targetY, int mapW, int mapH, std::function<int(int, int)> costFunc) {
    std::vector<std::pair<int, int>> path;
    if (startX == targetX && startY == targetY) return path;

    std::vector<std::vector<bool>> closedList(mapH, std::vector<bool>(mapW, false));
    std::priority_queue<std::shared_ptr<AStarNode>, std::vector<std::shared_ptr<AStarNode>>, CompareNode> openList;

    int startH = std::abs(startX - targetX) + std::abs(startY - targetY);
    openList.push(std::make_shared<AStarNode>(AStarNode{ startX, startY, 0, startH, nullptr }));

    while (!openList.empty()) {
        auto current = openList.top();
        openList.pop();

        if (closedList[current->y][current->x]) continue;
        closedList[current->y][current->x] = true;

        if (current->x == targetX && current->y == targetY) {
            auto currNode = current;
            while (currNode->parent != nullptr) {
                path.push_back({ currNode->x, currNode->y });
                currNode = currNode->parent;
            }
            std::reverse(path.begin(), path.end());
            return path;
        }

        for (const auto& off : kCardinalOffsets) {
            int nx = current->x + off.dx;
            int ny = current->y + off.dy;

            if (nx >= 0 && nx < mapW && ny >= 0 && ny < mapH) {
                int cost = costFunc(nx, ny);
                if (cost >= 0 && !closedList[ny][nx]) {
                    int g = current->g + cost;
                    int h = std::abs(nx - targetX) + std::abs(ny - targetY);
                    openList.push(std::make_shared<AStarNode>(AStarNode{ nx, ny, g, h, current }));
                }
            }
        }
    }
    return path;
}

bool AIManager::TryPlanBombEscape(int botX, int botY, int mapW, int mapH,
                                  const LevelManager& lm, const BombManager& bm, int fp,
                                  const std::function<int(int, int)>& walkCost,
                                  std::pair<int, int>& outFirstStep) {
    // 放彈前確認逃生路徑「不只存在，還走得到」。escape path 只避開「已存在」的炸彈/爆炸與
    // 危險格，不避開這顆 pretend bomb 自己的火力 — 它有 3 秒引信，bot 來得及穿出去。
    auto testSafe = m_DangerMap.FindSafeSpot(botX, botY, lm, bm, fp, botX, botY);
    if (!testSafe.found) return false;

    auto escapePath = FindPath(botX, botY, testSafe.x, testSafe.y, mapW, mapH, walkCost);
    if (escapePath.empty()) return false;

    outFirstStep = escapePath[0];
    // 通知後續 bot：這顆 pending 炸彈的爆炸範圍他們也要避開，避免多 bot 同時放彈互炸
    m_DangerMap.RegisterPendingBomb(botX, botY, fp, lm);
    return true;
}

void AIManager::Update(std::vector<std::shared_ptr<Player>>& players,
    const LevelManager& levelManager,
    const BombManager& bombManager,
    const InteractableManager& interactableManager,
    const std::vector<std::shared_ptr<Spirit>>& spirits,
    const TurretManager& turretManager) {

    m_DangerMap.Rebuild(levelManager, bombManager);

    const auto& interactables = interactableManager.GetInteractables();
    int mapW = levelManager.GetMapWidth();
    int mapH = levelManager.GetMapHeight();

    // 跨 bot 共享的目標鎖定表：本幀內某物件被某 bot 鎖定後，其他 bot 不再追同一物
    std::unordered_set<Interactable*> claimedTargets;

    for (auto& bot : players) {
        if (bot->IsDead()) continue;
        // Cross-cast 到 IProgrammableController：HumanController 沒有實作此介面所以會回 nullptr，
        // 自動被略過 — 不再依賴具體 BotController 類別。
        auto botController = dynamic_cast<IProgrammableController*>(bot->GetController());
        if (!botController) continue;

        botController->TickCooldown();

        int botX = bot->GetGridX();
        int botY = bot->GetGridY();
        int botFirepower = bot->GetFirepower();

        // 本幀尋路所需的環境查詢與成本函式都集中在 nav (取代散落的 lambda)
        BotNavigator nav(levelManager, bombManager, m_DangerMap, spirits, turretManager, players, bot.get(), botFirepower);
        auto safeCost = [&](int x, int y) { return nav.SafeWalkCost(x, y); };

        // 反應延遲：cooldown 中跳過決策，input 保持上一次設定。
        // 下列情況必須 bypass、每幀重決策：
        //  (a) 身處危險 — 100ms 反應太慢可能直接被炸死
        //  (b) 站在輸送帶上 — 力場每幀都在推，需要即時調整方向鍵
        //  (c) bot 像素位置已偏離當前格中心 (>2 px) — 必須每幀微修否則就 S 型走
        const bool inDanger = m_DangerMap.IsLethal(botX, botY, levelManager, botFirepower);
        const auto forceHere = interactableManager.GetForceAt(botX, botY);
        const bool onForceField = (forceHere.x != 0.0f || forceHere.y != 0.0f);

        const auto botPixel = bot->GetPixelPos();
        const float curCenterX = GridCoord::ToPixelX(botX);
        const float curCenterY = GridCoord::ToPixelY(botY);
        const bool needsAlignment = std::abs(botPixel.x - curCenterX) > 2.0f
                                 || std::abs(botPixel.y - curCenterY) > 2.0f;

        const bool needsImmediate = inDanger || onForceField || needsAlignment;
        if (!needsImmediate && !botController->IsReadyToDecide()) continue;

        // 決策完成後設定下次冷卻 (用 playerID 做相位偏移，分散不同 bot 的決策幀)
        // immediate 情境 (危險 / 輸送帶 / 像素偏離) cooldown = 0，每幀都會繼續微調 — 避免 S 型走
        struct ResetGuard {
            IProgrammableController* ctrl;
            int frames;
            ~ResetGuard() { ctrl->ResetCooldown(frames); }
        };
        const int nextCooldown = needsImmediate
            ? 0
            : Constants::Bot::kReactionFrames + (bot->GetPlayerID() % 3);
        ResetGuard guard{ botController, nextCooldown };

        // 策略 1：身處危險 — 逃往最近的安全格 (穿越危險也要逃，故用 RetreatCost)
        if (inDanger) {
            auto safeSpot = m_DangerMap.FindSafeSpot(botX, botY, levelManager, bombManager, botFirepower);
            if (safeSpot.found) {
                auto path = FindPath(botX, botY, safeSpot.x, safeSpot.y, mapW, mapH,
                                     [&](int x, int y) { return nav.RetreatCost(x, y); });
                if (!path.empty()) ExecuteMove(botController, botX, botY, path[0].first, path[0].second, false,
                                                bot->GetPixelPos());
            }
            continue;
        }

        // 找最近的敵方 defender
        std::shared_ptr<Player> targetDefender = nullptr;
        int defenderDist = 999;
        for (const auto& p : players) {
            if (p->GetTeam() == Team::DEFENDER && !p->IsDead()) {
                int dist = std::abs(p->GetGridX() - botX) + std::abs(p->GetGridY() - botY);
                if (dist < defenderDist) {
                    defenderDist = dist;
                    targetDefender = p;
                }
            }
        }

        // 目標選擇：優先撿物件 (priority hook)，否則追最近 defender
        int targetX = -1, targetY = -1;
        auto nearestTarget = FindNearestTarget(botX, botY, bot->HasKey(), interactables, claimedTargets);

        if (nearestTarget) {
            targetX = nearestTarget->GetGridX();
            targetY = nearestTarget->GetGridY();
            claimedTargets.insert(nearestTarget.get());  // 鎖定，後續 bot 不重複追
        } else if (targetDefender) {
            targetX = targetDefender->GetGridX();
            targetY = targetDefender->GetGridY();
        }

        if (targetX == -1) {
            botController->SetInput(false, false, false, false, false);
            continue;
        }

        // 策略 3：有安全路徑可達目標
        auto pathSafe = FindPath(botX, botY, targetX, targetY, mapW, mapH, safeCost);
        if (!pathSafe.empty()) {
            // 「想要放炸彈」的觸發條件：能炸到 defender 或精靈
            bool wantBomb = false;
            if (targetDefender && targetX == targetDefender->GetGridX() && targetY == targetDefender->GetGridY()) {
                if (nav.BombReaches(botX, botY, targetDefender->GetGridX(), targetDefender->GetGridY())) {
                    wantBomb = true;
                }
            }
            if (!wantBomb && nav.BombHitsAnySpirit(botX, botY)) {
                wantBomb = true;
            }

            // 預設朝目標走；想放彈則先驗證逃生路徑，可行才放並改朝逃生點走 (否則自己走進爆炸圈)
            bool placeBomb = false;
            int moveToX = pathSafe[0].first;
            int moveToY = pathSafe[0].second;
            if (wantBomb) {
                std::pair<int, int> escapeStep;
                if (TryPlanBombEscape(botX, botY, mapW, mapH, levelManager, bombManager, botFirepower, safeCost, escapeStep)) {
                    placeBomb = true;
                    moveToX = escapeStep.first;
                    moveToY = escapeStep.second;
                }
            }

            ExecuteMove(botController, botX, botY, moveToX, moveToY, placeBomb, bot->GetPixelPos());
            continue;
        }

        // 策略 4：無安全路徑 — 嘗試炸牆開路
        auto pathThroughBricks = FindPath(botX, botY, targetX, targetY, mapW, mapH,
                                          [&](int x, int y) { return nav.BrickCost(x, y); });
        if (!pathThroughBricks.empty()) {
            int walkToX = botX, walkToY = botY;
            for (auto& p : pathThroughBricks) {
                if (levelManager.IsBrick(p.first, p.second)) break;
                walkToX = p.first; walkToY = p.second;
            }

            if (botX == walkToX && botY == walkToY) {
                // 已站在 brick 前：驗證逃生路徑走得到，可行才炸並朝逃生點走
                std::pair<int, int> escapeStep;
                if (TryPlanBombEscape(botX, botY, mapW, mapH, levelManager, bombManager, botFirepower, safeCost, escapeStep)) {
                    ExecuteMove(botController, botX, botY, escapeStep.first, escapeStep.second, true, bot->GetPixelPos());
                }
                else {
                    botController->SetInput(false, false, false, false, false);
                }
            }
            else {
                auto pathToBrick = FindPath(botX, botY, walkToX, walkToY, mapW, mapH, safeCost);
                if (!pathToBrick.empty()) ExecuteMove(botController, botX, botY, pathToBrick[0].first, pathToBrick[0].second, false,
                                                       bot->GetPixelPos());
                else botController->SetInput(false, false, false, false, false);
            }
            continue;
        }

        // 策略 5：無路可走 — 自殺攻擊 (無視火焰，砲台仍是物理碰撞穿不過)
        auto pathIgnoreFire = FindPath(botX, botY, targetX, targetY, mapW, mapH,
                                       [&](int x, int y) { return nav.SuicideCost(x, y); });
        if (!pathIgnoreFire.empty()) {
            // 僅在「對 defender 有直線視野且夠近」時才值得自殺放彈，且仍要逃生路徑走得到
            if (targetDefender && defenderDist <= 5 &&
                nav.HasLineOfSight(botX, botY, targetDefender->GetGridX(), targetDefender->GetGridY())) {
                std::pair<int, int> escapeStep;
                if (TryPlanBombEscape(botX, botY, mapW, mapH, levelManager, bombManager, botFirepower, safeCost, escapeStep)) {
                    ExecuteMove(botController, botX, botY, escapeStep.first, escapeStep.second, true, bot->GetPixelPos());
                    continue;
                }
            }
            botController->SetInput(false, false, false, false, false);
            continue;
        }

        botController->SetInput(false, false, false, false, false);
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

void AIManager::ExecuteMove(IProgrammableController* botController, int fromX, int fromY, int toX, int toY, bool placeBomb,
                             glm::vec2 botPixelPos) const {
    bool up = false, down = false, left = false, right = false;
    const float targetPixelX = GridCoord::ToPixelX(toX);
    const float targetPixelY = GridCoord::ToPixelY(toY);
    constexpr float kAlignTolerance = 2.0f;  // 像素級漂移容忍度

    // 主方向 (toX != fromX 或 toY != fromY) + 副方向 (校正另一軸的漂移)
    if (toX != fromX) {
        // 水平主移動：另外看 Y 軸有沒有漂走，有就補 up/down 拉回中線
        if (toX > fromX) right = true;
        else             left  = true;

        if (botPixelPos.y < targetPixelY - kAlignTolerance)       up   = true;
        else if (botPixelPos.y > targetPixelY + kAlignTolerance)  down = true;
    }
    else if (toY != fromY) {
        // 垂直主移動：另外看 X 軸有沒有漂走，有就補 left/right
        if (toY > fromY) down = true;
        else             up   = true;

        if (botPixelPos.x < targetPixelX - kAlignTolerance)       right = true;
        else if (botPixelPos.x > targetPixelX + kAlignTolerance)  left  = true;
    }

    botController->SetInput(up, down, left, right, placeBomb);
}
