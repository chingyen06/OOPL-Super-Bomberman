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
#include "Controller/CooldownResetGuard.hpp"
#include <queue>
#include <cmath>
#include <algorithm>
#include <functional>
#include <map>

bool AIManager::TryPlanBombEscape(int botX, int botY, int mapW, int mapH,
                                  const LevelManager& lm, const BombManager& bm, int fp,
                                  const std::function<int(int, int)>& walkCost,
                                  std::pair<int, int>& outFirstStep) {
    // 放彈前確認逃生路徑「不只存在，還走得到」。escape path 只避開「已存在」的炸彈/爆炸與
    // 危險格，不避開這顆 pretend bomb 自己的火力 — 它有 3 秒引信，bot 來得及穿出去。
    auto testSafe = m_DangerMap.FindSafeSpot(botX, botY, lm, bm, fp, botX, botY);
    if (!testSafe.Found()) return false;

    auto escapePath = m_Pathfinder.FindPath(botX, botY, testSafe.X(), testSafe.Y(), mapW, mapH, walkCost);
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

    // 先做目標分派：保留各 bot 仍有效的承諾，再把沒目標的指派到最近未佔用物件。
    // 之後決策迴圈只「讀取」各 bot 已分派好的目標 → 目標穩定 (不抽搐) 且彼此不同 (不浪費)。
    AssignTargets(players, interactables);

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
            // 每隻 bot 對每格加 0~2 的固定偏好擾動：不同 bot 偏好不同走法 → 路線分散、不會整排
            // 走同一條。只依 (id,x,y) 不依時間，所以路徑逐幀穩定、不抽搐。
            const unsigned h = wanderSeed ^ (static_cast<unsigned>(x) * 73856093u)
                                          ^ (static_cast<unsigned>(y) * 19349663u);
            return c + static_cast<int>(h % 3u);
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

        // 決策完成後設定下次冷卻 (用 playerID 做相位偏移，分散不同 bot 的決策幀)。
        // immediate 情境 (危險 / 輸送帶 / 像素偏離) cooldown = 0，每幀都會繼續微調 — 避免 S 型走。
        // RAII 守衛已抽到 CooldownResetGuard (header)，不再於本 .cpp 內定義區域類別。
        // 反應幀數改由性格決定：積極的 bot 反應快、較少在原地發呆，謹慎的較慢。
        const int nextCooldown = needsImmediate
            ? 0
            : profile.ReactionFrames() + (bot->GetPlayerID() % 3);
        CooldownResetGuard guard{ botController, nextCooldown };

        // 策略 1：身處危險 — 逃往最近的安全格 (穿越危險也要逃，故用 RetreatCost)
        if (inDanger) {
            auto safeSpot = m_DangerMap.FindSafeSpot(botX, botY, levelManager, bombManager, botFirepower);
            if (safeSpot.Found()) {
                auto path = m_Pathfinder.FindPath(botX, botY, safeSpot.X(), safeSpot.Y(), mapW, mapH,
                                     [&](int x, int y) { return nav.RetreatCost(x, y); });
                if (!path.empty()) ExecuteMove(botController, botX, botY, path[0].first, path[0].second, false, forceHere);
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
        // 目標已由 AssignTargets 分派好 (每隻 bot 各異且穩定)；這裡只讀取。
        // 有分派到物件 → 追該物件；沒有 (物件比 bot 少) → 去追防守方。
        int targetX = -1, targetY = -1;
        auto chosenTarget = FindTargetAt(botController->GoalX(), botController->GoalY(), bot->HasKey(), interactables);
        if (chosenTarget) {
            targetX = chosenTarget->GetGridX();
            targetY = chosenTarget->GetGridY();
        } else if (targetDefender) {
            targetX = targetDefender->GetGridX();
            targetY = targetDefender->GetGridY();
        }

        if (targetX == -1) {
            botController->SetInput(false, false, false, false, false);
            continue;
        }

        // 策略 3：有安全路徑可達目標
        auto pathSafe = m_Pathfinder.FindPath(botX, botY, targetX, targetY, mapW, mapH, safeCost);
        if (!pathSafe.empty()) {
            const int defX = targetDefender ? targetDefender->GetGridX() : -999;
            const int defY = targetDefender ? targetDefender->GetGridY() : -999;
            const bool defenderOnTarget = targetDefender && targetX == defX && targetY == defY;

            // 「想要放炸彈」的觸發條件。重點：只有「真的在追防守方」(沒有物件目標) 時，才會
            // 因為目標格上站著防守方而開炸；若我的目標是寶箱/鑰匙，就算防守方站在上面，也是
            // 走過去「開」它而不是炸它 (玩家不互相碰撞，可同格) — 修掉「站在寶箱上卡住 AI」。
            bool wantBomb = false;
            if (!chosenTarget && defenderOnTarget && nav.BombReaches(botX, botY, defX, defY)) {
                wantBomb = true;
            }
            if (!wantBomb && nav.BombHitsAnySpirit(botX, botY)) {
                wantBomb = true;  // 順手炸到精靈 → 清路
            }
            // 好戰性格順手佈彈逼退防守方；但若防守方正站在我的物件目標上，別炸 (要去開它)。
            if (!wantBomb && profile.HuntsDefender() && targetDefender && !defenderOnTarget &&
                defenderDist <= profile.BombChaseRange() &&
                nav.HasLineOfSight(botX, botY, defX, defY) &&
                nav.BombReaches(botX, botY, defX, defY)) {
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

            ExecuteMove(botController, botX, botY, moveToX, moveToY, placeBomb, forceHere);
            continue;
        }

        // 策略 3.5：衝向「物件目標」(鑰匙 / 寶箱 / 道具)。沒有完全安全路徑時，若判斷「能趕在
        // 火焰蔓延到之前」抵達，就算路上有即將爆炸 (尚未噴火) 的格也衝過去 —— 例如趕著開寶箱。
        // 逐格比對「該格還有幾 frame 變致命」與「bot 走到該格約需幾 frame」，全部來得及才衝。
        // 只在追物件、且性格「敢冒險」時啟用 (追防守方不冒這個險)；不會踩進「正在燒」的火焰
        // (RushCost 已擋)。多數性格不衝 → 防守方在寶箱旁佈彈/用武器就能嚇阻、守得住。
        if (chosenTarget && profile.RushesObjectives()) {
            auto rushPath = m_Pathfinder.FindPath(botX, botY, targetX, targetY, mapW, mapH,
                                     [&](int x, int y) { return nav.RushCost(x, y); });
            if (!rushPath.empty()) {
                constexpr int kFramesPerCell = 12;  // ~32px / 3px每幀
                constexpr int kMargin = 6;          // 安全餘量
                bool inTime = true;
                for (int i = 0; i < static_cast<int>(rushPath.size()); ++i) {
                    const int arrive = (i + 1) * kFramesPerCell;
                    if (bombManager.FramesUntilLethalAt(rushPath[i].first, rushPath[i].second, levelManager)
                            <= arrive + kMargin) { inTime = false; break; }
                }
                if (inTime) {
                    ExecuteMove(botController, botX, botY, rushPath[0].first, rushPath[0].second, false, forceHere);
                    continue;
                }
            }
        }

        // 策略 4：無安全路徑 — 嘗試炸牆開路
        auto pathThroughBricks = m_Pathfinder.FindPath(botX, botY, targetX, targetY, mapW, mapH,
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
                    ExecuteMove(botController, botX, botY, escapeStep.first, escapeStep.second, true, forceHere);
                }
                else {
                    botController->SetInput(false, false, false, false, false);
                }
            }
            else {
                auto pathToBrick = m_Pathfinder.FindPath(botX, botY, walkToX, walkToY, mapW, mapH, safeCost);
                if (!pathToBrick.empty()) ExecuteMove(botController, botX, botY, pathToBrick[0].first, pathToBrick[0].second, false, forceHere);
                else botController->SetInput(false, false, false, false, false);
            }
            continue;
        }

        // 策略 5：對防守方自殺式突擊 (僅「真的在追防守方」、有視線且夠近時；無視火焰衝過去開炸)。
        if (targetDefender && defenderDist <= 5 + profile.SuicideBoldness() &&
            nav.HasLineOfSight(botX, botY, targetDefender->GetGridX(), targetDefender->GetGridY())) {
            auto pathIgnoreFire = m_Pathfinder.FindPath(botX, botY, targetX, targetY, mapW, mapH,
                                           [&](int x, int y) { return nav.SuicideCost(x, y); });
            if (!pathIgnoreFire.empty()) {
                std::pair<int, int> escapeStep;
                if (TryPlanBombEscape(botX, botY, mapW, mapH, levelManager, bombManager, botFirepower, safeCost, escapeStep)) {
                    ExecuteMove(botController, botX, botY, escapeStep.first, escapeStep.second, true, forceHere);
                    continue;
                }
            }
        }

        // 最後手段：無法安全抵達目標時，靠近到「安全可達、離目標最近」的格並等待，
        // 而不是直接放棄、原地發呆 (例如寶箱旁有人放炸彈，先靠過去等火焰過再開)。
        if (!ApproachTarget(botController, botX, botY, targetX, targetY, mapW, mapH, safeCost, forceHere)) {
            botController->SetInput(false, false, false, false, false);
        }
    }
}

bool AIManager::ApproachTarget(IProgrammableController* botController, int botX, int botY, int targetX, int targetY,
                               int mapW, int mapH, const std::function<int(int, int)>& safeCost, glm::vec2 force) {
    // BFS：從 bot 在「安全可走」格上擴散，記錄離目標 (曼哈頓) 最近的可達格。
    std::vector<std::vector<bool>> seen(mapH, std::vector<bool>(mapW, false));
    std::queue<std::pair<int, int>> q;
    q.push({ botX, botY });
    seen[botY][botX] = true;
    int bestX = botX, bestY = botY, bestD = std::abs(botX - targetX) + std::abs(botY - targetY);

    while (!q.empty()) {
        const auto cur = q.front(); q.pop();
        for (const auto& off : kCardinalOffsets) {
            const int nx = cur.first + off.dx, ny = cur.second + off.dy;
            if (nx < 0 || nx >= mapW || ny < 0 || ny >= mapH || seen[ny][nx]) continue;
            if (safeCost(nx, ny) < 0) continue;  // 不可安全走
            seen[ny][nx] = true;
            q.push({ nx, ny });
            const int d = std::abs(nx - targetX) + std::abs(ny - targetY);
            if (d < bestD) { bestD = d; bestX = nx; bestY = ny; }
        }
    }

    if (bestX == botX && bestY == botY) return false;  // 已是最靠近的位置，無處可再靠近
    auto path = m_Pathfinder.FindPath(botX, botY, bestX, bestY, mapW, mapH, safeCost);
    if (path.empty()) return false;
    ExecuteMove(botController, botX, botY, path[0].first, path[0].second, false, force);
    return true;
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

    // Pass 1：保留「仍有效」的承諾 (該格物件還在、對該 bot 仍有效、且未被更前面的 bot 佔走)。
    // 失效或撞車的 → 清掉 goal，留待 Pass 2 重新指派。
    for (auto& b : players) {
        if (b->IsDead()) continue;
        auto ctrl = dynamic_cast<IProgrammableController*>(b->GetController());
        if (!ctrl) continue;
        auto t = FindTargetAt(ctrl->GoalX(), ctrl->GoalY(), b->HasKey(), items);
        if (t && !claimed.count(t.get())) claimed.insert(t.get());  // 保留此承諾並佔用
        else ctrl->SetGoal(-1, -1);
    }

    // Pass 2：沒有效承諾的 bot → 指派「最近、尚未被佔用」的物件，確保每隻追不同目標。
    for (auto& b : players) {
        if (b->IsDead()) continue;
        auto ctrl = dynamic_cast<IProgrammableController*>(b->GetController());
        if (!ctrl || ctrl->GoalX() >= 0) continue;  // 已有承諾
        auto t = FindNearestTarget(b->GetGridX(), b->GetGridY(), b->HasKey(), items, claimed);
        if (t) { ctrl->SetGoal(t->GetGridX(), t->GetGridY()); claimed.insert(t.get()); }
        // 沒物件可派 → goal 維持 -1，決策迴圈會改去追防守方
    }
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

void AIManager::ExecuteMove(IProgrammableController* botController, int fromX, int fromY, int toX, int toY,
                            bool placeBomb, glm::vec2 force) const {
    bool up = false, down = false, left = false, right = false;

    // 主方向：朝目標格 (路徑每步皆為上下左右其一)。一般地面只送這個方向——另一軸的
    // 置中交給 Player 的自動歸位 (有 clamp、不過衝)，避免中線抽搐。
    if (toX != fromX) {
        if (toX > fromX) right = true;
        else             left  = true;
        // 在輸送帶上橫向移動時，Player 不會對「有帶力的那一軸」自動歸位，會被帶偏離本列
        // 而過不去轉角。這裡主動補按與上下帶力相反的方向抵銷之 (force.y>0 表示往上推)。
        if (force.y < 0.0f)      down = false, up = true;
        else if (force.y > 0.0f) down = true;
    }
    else if (toY != fromY) {
        if (toY > fromY) down = true;
        else             up   = true;
        if (force.x < 0.0f)      right = true;   // 帶往左推 → 補按右
        else if (force.x > 0.0f) left = true;    // 帶往右推 → 補按左
    }

    botController->SetInput(up, down, left, right, placeBomb);
}
