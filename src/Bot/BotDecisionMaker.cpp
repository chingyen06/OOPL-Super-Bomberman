#include "Bot/BotDecisionMaker.hpp"

#include <algorithm>
#include <cmath>
#include <queue>
#include <vector>

#include "Bot/BotNavigator.hpp"
#include "Bot/DangerMap.hpp"
#include "Bot/Pathfinder.hpp"
#include "BombManager.hpp"
#include "Controller/CooldownResetGuard.hpp"
#include "Controller/IProgrammableController.hpp"
#include "GameTypes.hpp"
#include "GridCoord.hpp"
#include "Interactable.hpp"
#include "InteractableManager.hpp"
#include "LevelManager.hpp"
#include "Player.hpp"
#include "Spirit.hpp"
#include "Turret/TurretManager.hpp"

BotDecisionMaker::BotDecisionMaker(const Pathfinder& pathfinder, DangerMap& dangerMap)
    : m_Pathfinder(pathfinder), m_DangerMap(dangerMap) {}

bool BotDecisionMaker::TryPlanBombEscape(int botX, int botY, int mapW, int mapH,
                                         const LevelManager& lm, const BombManager& bm, int fp,
                                         const std::function<int(int, int)>& walkCost,
                                         std::pair<int, int>& outFirstStep) const {
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

std::shared_ptr<Interactable> BotDecisionMaker::FindTargetAt(int gridX, int gridY, bool hasKey,
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

void BotDecisionMaker::ExecuteMove(IProgrammableController& controller, int fromX, int fromY,
                                   int toX, int toY, bool placeBomb, glm::vec2 force) const {
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
        if (force.x < 0.0f)      right = true;
        else if (force.x > 0.0f) left = true;
    }

    controller.SetInput(up, down, left, right, placeBomb);
}

bool BotDecisionMaker::ApproachTarget(IProgrammableController& controller, int botX, int botY,
                                      int targetX, int targetY, int mapW, int mapH,
                                      const std::function<int(int, int)>& safeCost, glm::vec2 force) const {
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
    ExecuteMove(controller, botX, botY, path[0].first, path[0].second, false, force);
    return true;
}

void BotDecisionMaker::DecideForBot(Player& bot,
                                    IProgrammableController& botController,
                                    const std::vector<std::shared_ptr<Interactable>>& interactables,
                                    const std::vector<std::shared_ptr<Player>>& players,
                                    const LevelManager& levelManager,
                                    const BombManager& bombManager,
                                    const InteractableManager& interactableManager,
                                    const std::vector<std::shared_ptr<Spirit>>& spirits,
                                    const TurretManager& turretManager) {
    const int mapW = levelManager.GetMapWidth();
    const int mapH = levelManager.GetMapHeight();

    botController.TickCooldown();

    const IBotProfile& profile = botController.Profile();

    const int botX = bot.GetGridX();
    const int botY = bot.GetGridY();
    const int botFirepower = bot.GetFirepower();

    BotNavigator nav(levelManager, bombManager, m_DangerMap, spirits, turretManager, players, &bot, botFirepower);

    // 每隻 bot 一個固定的「繞路偏好」種子：等長的多條安全路線中，不同 bot 偏好不同走法，
    // 整體動線分散、不機械。微擾只依 (id,x,y)、不依時間 → 同幀路徑穩定、不抽搐。
    const unsigned wanderSeed = static_cast<unsigned>(bot.GetPlayerID()) * 2654435761u;
    auto safeCost = [&, wanderSeed](int x, int y) {
        const int c = nav.SafeWalkCost(x, y);
        if (c < 0) return c;
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

    const auto botPixel = bot.GetPixelPos();
    const float curCenterX = GridCoord::ToPixelX(botX);
    const float curCenterY = GridCoord::ToPixelY(botY);
    const bool needsAlignment = std::abs(botPixel.x - curCenterX) > 2.0f
                             || std::abs(botPixel.y - curCenterY) > 2.0f;

    const bool needsImmediate = inDanger || onForceField || needsAlignment;
    if (!needsImmediate && !botController.IsReadyToDecide()) return;

    // 決策完成後設定下次冷卻 (用 playerID 做相位偏移分散決策幀)。immediate 情境 cooldown = 0。
    const int nextCooldown = needsImmediate
        ? 0
        : profile.ReactionFrames() + (bot.GetPlayerID() % 3);
    CooldownResetGuard guard{ &botController, nextCooldown };

    // 策略 1：身處危險 — 逃往最近的安全格 (穿越危險也要逃，故用 RetreatCost)
    if (inDanger) {
        auto safeSpot = m_DangerMap.FindSafeSpot(botX, botY, levelManager, bombManager, botFirepower);
        if (safeSpot.Found()) {
            auto path = m_Pathfinder.FindPath(botX, botY, safeSpot.X(), safeSpot.Y(), mapW, mapH,
                                 [&](int x, int y) { return nav.RetreatCost(x, y); });
            if (!path.empty()) ExecuteMove(botController, botX, botY, path[0].first, path[0].second, false, forceHere);
        }
        return;
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

    // 目標已由 AIManager::AssignTargets 分派 (每隻 bot 不同且穩定)；這裡只讀取。
    int targetX = -1, targetY = -1;
    auto chosenTarget = FindTargetAt(botController.GoalX(), botController.GoalY(), bot.HasKey(), interactables);
    if (chosenTarget) {
        targetX = chosenTarget->GetGridX();
        targetY = chosenTarget->GetGridY();
    } else if (targetDefender) {
        targetX = targetDefender->GetGridX();
        targetY = targetDefender->GetGridY();
    }

    if (targetX == -1) {
        botController.SetInput(false, false, false, false, false);
        return;
    }

    // 策略 3：有安全路徑可達目標
    auto pathSafe = m_Pathfinder.FindPath(botX, botY, targetX, targetY, mapW, mapH, safeCost);
    if (!pathSafe.empty()) {
        const int defX = targetDefender ? targetDefender->GetGridX() : -999;
        const int defY = targetDefender ? targetDefender->GetGridY() : -999;
        const bool defenderOnTarget = targetDefender && targetX == defX && targetY == defY;

        // 「想要放炸彈」的觸發條件。重點：只有「真的在追防守方」(沒有物件目標) 時，才會
        // 因為目標格上站著防守方而開炸；若我的目標是寶箱/鑰匙，就算防守方站在上面，也是
        // 走過去「開」它而不是炸它 (玩家不互相碰撞，可同格)。
        bool wantBomb = false;
        if (!chosenTarget && defenderOnTarget && nav.BombReaches(botX, botY, defX, defY)) {
            wantBomb = true;
        }
        if (!wantBomb && nav.BombHitsAnySpirit(botX, botY)) {
            wantBomb = true;  // 順手炸到精靈 → 清路
        }
        if (!wantBomb && profile.HuntsDefender() && targetDefender && !defenderOnTarget &&
            defenderDist <= profile.BombChaseRange() &&
            nav.HasLineOfSight(botX, botY, defX, defY) &&
            nav.BombReaches(botX, botY, defX, defY)) {
            wantBomb = true;
        }

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
        return;
    }

    // 策略 3.5：衝向物件目標 (鑰匙 / 寶箱 / 道具)，僅敢冒險的性格啟用。
    if (chosenTarget && profile.RushesObjectives()) {
        auto rushPath = m_Pathfinder.FindPath(botX, botY, targetX, targetY, mapW, mapH,
                                 [&](int x, int y) { return nav.RushCost(x, y); });
        if (!rushPath.empty()) {
            constexpr int kFramesPerCell = 12;
            constexpr int kMargin = 6;
            bool inTime = true;
            for (int i = 0; i < static_cast<int>(rushPath.size()); ++i) {
                const int arrive = (i + 1) * kFramesPerCell;
                if (bombManager.FramesUntilLethalAt(rushPath[i].first, rushPath[i].second, levelManager)
                        <= arrive + kMargin) { inTime = false; break; }
            }
            if (inTime) {
                ExecuteMove(botController, botX, botY, rushPath[0].first, rushPath[0].second, false, forceHere);
                return;
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
            std::pair<int, int> escapeStep;
            if (TryPlanBombEscape(botX, botY, mapW, mapH, levelManager, bombManager, botFirepower, safeCost, escapeStep)) {
                ExecuteMove(botController, botX, botY, escapeStep.first, escapeStep.second, true, forceHere);
            }
            else {
                botController.SetInput(false, false, false, false, false);
            }
        }
        else {
            auto pathToBrick = m_Pathfinder.FindPath(botX, botY, walkToX, walkToY, mapW, mapH, safeCost);
            if (!pathToBrick.empty()) ExecuteMove(botController, botX, botY, pathToBrick[0].first, pathToBrick[0].second, false, forceHere);
            else botController.SetInput(false, false, false, false, false);
        }
        return;
    }

    // 策略 5：對防守方自殺式突擊 (有視線且夠近時；無視火焰衝過去開炸)。
    if (targetDefender && defenderDist <= 5 + profile.SuicideBoldness() &&
        nav.HasLineOfSight(botX, botY, targetDefender->GetGridX(), targetDefender->GetGridY())) {
        auto pathIgnoreFire = m_Pathfinder.FindPath(botX, botY, targetX, targetY, mapW, mapH,
                                       [&](int x, int y) { return nav.SuicideCost(x, y); });
        if (!pathIgnoreFire.empty()) {
            std::pair<int, int> escapeStep;
            if (TryPlanBombEscape(botX, botY, mapW, mapH, levelManager, bombManager, botFirepower, safeCost, escapeStep)) {
                ExecuteMove(botController, botX, botY, escapeStep.first, escapeStep.second, true, forceHere);
                return;
            }
        }
    }

    // 最後手段：靠近並等待。
    if (!ApproachTarget(botController, botX, botY, targetX, targetY, mapW, mapH, safeCost, forceHere)) {
        botController.SetInput(false, false, false, false, false);
    }
}
