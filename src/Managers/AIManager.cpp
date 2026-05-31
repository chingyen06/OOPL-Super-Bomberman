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

class AStarNode {
public:
    int x, y;
    int g, h;
    std::shared_ptr<AStarNode> parent;
    int f() const { return g + h; }
};

class CompareNode {
public:
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

        const IBotProfile& profile = botController->Profile();  // 這隻 bot 的性格 / 想法

        int botX = bot->GetGridX();
        int botY = bot->GetGridY();
        int botFirepower = bot->GetFirepower();

        // 本幀尋路所需的環境查詢與成本函式都集中在 nav (取代散落的 lambda)
        BotNavigator nav(levelManager, bombManager, m_DangerMap, spirits, turretManager, players, bot.get(), botFirepower);

        // 每隻 bot 一個固定的「繞路偏好」種子：在等長的多條安全路線中，不同 bot 會偏好
        // 不同走法，使整體動線較不機械、不一致 (不會全擠同一條路)。微擾只依 (id,x,y)、不依
        // 時間，所以同一幀到同一目標的路徑穩定、逐幀一致，不會造成抽搐。-1 (不可走) 不加擾。
        const unsigned wanderSeed = static_cast<unsigned>(bot->GetPlayerID()) * 2654435761u;
        auto safeCost = [&, wanderSeed](int x, int y) {
            const int c = nav.SafeWalkCost(x, y);
            if (c < 0) return c;
            const unsigned h = wanderSeed ^ (static_cast<unsigned>(x) * 73856093u)
                                          ^ (static_cast<unsigned>(y) * 19349663u);
            return c + static_cast<int>(h & 1u);
        };

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
        class ResetGuard {
        public:
            IProgrammableController* ctrl;
            int frames;
            ~ResetGuard() { ctrl->ResetCooldown(frames); }
        };
        // 反應幀數改由性格決定：積極的 bot 反應快、較少在原地發呆，謹慎的較慢。
        const int nextCooldown = needsImmediate
            ? 0
            : profile.ReactionFrames() + (bot->GetPlayerID() % 3);
        ResetGuard guard{ botController, nextCooldown };

        // 策略 1：身處危險 — 逃往最近的安全格 (穿越危險也要逃，故用 RetreatCost)
        if (inDanger) {
            auto safeSpot = m_DangerMap.FindSafeSpot(botX, botY, levelManager, bombManager, botFirepower);
            if (safeSpot.found) {
                auto path = FindPath(botX, botY, safeSpot.x, safeSpot.y, mapW, mapH,
                                     [&](int x, int y) { return nav.RetreatCost(x, y); });
                if (!path.empty()) ExecuteMove(botController, botX, botY, path[0].first, path[0].second, false);
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

        // 目標選擇：一律優先撿鑰匙 / 道具 / 寶箱 — 這才是進攻方致勝的路 (沒鑰匙就開不了
        // 寶箱)。沒有可撿的物件時才去追防守方。性格差異改體現在「反應速度、佈彈積極度、
        // 自殺突擊膽量」上，而不是叫 bot 放著鑰匙不撿。
        //
        // 目標承諾 (anti-twitch)：先續追上次鎖定的物件 (該格仍有對本 bot 有效的物件、且
        // 未被別人 claim)；否則才重挑最近的。避免多隻 bot 互搶鄰近鑰匙時目標每幀互換、原地抽搐。
        int targetX = -1, targetY = -1;
        auto committed = FindTargetAt(botController->GoalX(), botController->GoalY(), bot->HasKey(), interactables);
        if (committed && claimedTargets.count(committed.get())) committed = nullptr;  // 已被別的 bot 鎖定
        auto chosenTarget = committed ? committed
                                      : FindNearestTarget(botX, botY, bot->HasKey(), interactables, claimedTargets);

        if (chosenTarget) {
            targetX = chosenTarget->GetGridX();
            targetY = chosenTarget->GetGridY();
            claimedTargets.insert(chosenTarget.get());  // 鎖定，後續 bot 不重複追
            botController->SetGoal(targetX, targetY);    // 記住，下一幀續追同一目標
        } else {
            botController->SetGoal(-1, -1);              // 沒有物件可撿 → 清除承諾
            if (targetDefender) {
                targetX = targetDefender->GetGridX();
                targetY = targetDefender->GetGridY();
            }
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
            // 好戰性格 (HuntsDefender)：就算正趕往物件，途中若防守方落在攻擊範圍 (BombChaseRange)
            // 內、有視線且火力掃得到，也會順手佈彈逼退 — 性格越好戰、範圍越大越敢炸。
            if (!wantBomb && profile.HuntsDefender() && targetDefender &&
                defenderDist <= profile.BombChaseRange() &&
                nav.HasLineOfSight(botX, botY, targetDefender->GetGridX(), targetDefender->GetGridY()) &&
                nav.BombReaches(botX, botY, targetDefender->GetGridX(), targetDefender->GetGridY())) {
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

            ExecuteMove(botController, botX, botY, moveToX, moveToY, placeBomb);
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
                    ExecuteMove(botController, botX, botY, escapeStep.first, escapeStep.second, true);
                }
                else {
                    botController->SetInput(false, false, false, false, false);
                }
            }
            else {
                auto pathToBrick = FindPath(botX, botY, walkToX, walkToY, mapW, mapH, safeCost);
                if (!pathToBrick.empty()) ExecuteMove(botController, botX, botY, pathToBrick[0].first, pathToBrick[0].second, false);
                else botController->SetInput(false, false, false, false, false);
            }
            continue;
        }

        // 策略 5：無路可走 — 自殺攻擊 (無視火焰，砲台仍是物理碰撞穿不過)
        auto pathIgnoreFire = FindPath(botX, botY, targetX, targetY, mapW, mapH,
                                       [&](int x, int y) { return nav.SuicideCost(x, y); });
        if (!pathIgnoreFire.empty()) {
            // 僅在「對 defender 有直線視野且夠近」時才值得自殺放彈，且仍要逃生路徑走得到。
            // 容許距離隨性格膽量加大：膽大的 bot 願意從更遠處拼命突擊。
            if (targetDefender && defenderDist <= 5 + profile.SuicideBoldness() &&
                nav.HasLineOfSight(botX, botY, targetDefender->GetGridX(), targetDefender->GetGridY())) {
                std::pair<int, int> escapeStep;
                if (TryPlanBombEscape(botX, botY, mapW, mapH, levelManager, bombManager, botFirepower, safeCost, escapeStep)) {
                    ExecuteMove(botController, botX, botY, escapeStep.first, escapeStep.second, true);
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

std::shared_ptr<Interactable> AIManager::FindTargetAt(int gridX, int gridY, bool hasKey,
                                                      const std::vector<std::shared_ptr<Interactable>>& items) const {
    if (gridX < 0 || gridY < 0) return nullptr;
    for (const auto& item : items) {
        if (item->GetGridX() == gridX && item->GetGridY() == gridY &&
            item->GetAttackerTargetPriority(hasKey) > 0) {
            return item;  // 該格上仍有對本 bot 有效的目標 (e.g. 鑰匙還在、寶箱還沒開)
        }
    }
    return nullptr;  // 目標已被撿走 / 開啟 / 失效 → 呼叫端會改挑新目標
}

void AIManager::ExecuteMove(IProgrammableController* botController, int fromX, int fromY, int toX, int toY, bool placeBomb) const {
    bool up = false, down = false, left = false, right = false;

    // 只下「主方向」鍵；另一軸的置中交給 Player 的自動歸位 (有 clamp、不會過衝)。
    // 不再在這裡用「未夾住的方向鍵」做垂直/水平校正 —— 那會與 Player 的歸位疊加而在
    // 中線兩側反覆過衝 (尤其加速時)，正是 bot 原地抽搐的主因。
    if (toX != fromX) {
        if (toX > fromX) right = true;
        else             left  = true;
    }
    else if (toY != fromY) {
        if (toY > fromY) down = true;
        else             up   = true;
    }

    botController->SetInput(up, down, left, right, placeBomb);
}
