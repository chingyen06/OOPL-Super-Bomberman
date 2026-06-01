#include "Bot/BotNavigator.hpp"

#include "GameTypes.hpp"
#include "LevelManager.hpp"
#include "BombManager.hpp"
#include "DangerMap.hpp"
#include "Spirit.hpp"
#include "Turret/TurretManager.hpp"
#include "Player.hpp"

BotNavigator::BotNavigator(const LevelManager& lm, const BombManager& bm, const DangerMap& danger,
                           const std::vector<std::shared_ptr<Spirit>>& spirits, const TurretManager& turrets,
                           const std::vector<std::shared_ptr<Player>>& players, const Player* self, int firepower)
    : m_Lm(lm), m_Bm(bm), m_Danger(danger), m_Spirits(spirits), m_Turrets(turrets),
      m_Players(players), m_Self(self), m_Fp(firepower) {}

bool BotNavigator::IsSpiritAt(int x, int y) const {
    for (const auto& s : m_Spirits) {
        if (s->ShouldDelete()) continue;
        if (s->GetGridX() == x && s->GetGridY() == y) return true;
    }
    return false;
}

bool BotNavigator::IsTurretAt(int x, int y) const {
    return m_Turrets.IsTurretAt(x, y);
}

bool BotNavigator::IsBlockedByOther(int x, int y) const {
    for (const auto& other : m_Players) {
        if (other.get() == m_Self) continue;
        if (other->IsDead()) continue;
        if (other->GetGridX() == x && other->GetGridY() == y) return true;
    }
    return false;
}

// 一般安全行走：不可走 / 炸彈 / 爆炸 / 致命格 / 精靈 / 砲台皆排除
int BotNavigator::SafeWalkCost(int x, int y) const {
    if (!m_Lm.IsWalkable(x, y) || m_Bm.IsBombAt(x, y) || m_Bm.HasExplosionAt(x, y)) return -1;
    if (m_Danger.IsLethal(x, y, m_Lm, m_Fp)) return -1;
    if (IsSpiritAt(x, y) || IsTurretAt(x, y)) return -1;
    return IsBlockedByOther(x, y) ? kOtherPlayerPenalty : 1;
}

// 衝目標用：擋住牆/磚/炸彈/「正在燒的火焰」與精靈/砲台，但允許走「即將爆炸、尚未噴火」
// 的格 (高成本)。是否真的衝，由 AIManager 依爆炸時限判斷；這裡只負責讓路徑可規劃出來。
int BotNavigator::RushCost(int x, int y) const {
    if (!m_Lm.IsWalkable(x, y) || m_Bm.IsBombAt(x, y) || m_Bm.HasExplosionAt(x, y)) return -1;
    if (IsSpiritAt(x, y) || IsTurretAt(x, y)) return -1;
    int base = IsBlockedByOther(x, y) ? kOtherPlayerPenalty : 1;
    if (m_Danger.IsLethal(x, y, m_Lm, m_Fp)) base += 6;  // 即將爆炸的格：可走但盡量少踩
    return base;
}

// 逃離危險用：同 SafeWalkCost 但「不」排除致命格 — 本來就要穿越危險逃出去
int BotNavigator::RetreatCost(int x, int y) const {
    if (!m_Lm.IsWalkable(x, y) || m_Bm.IsBombAt(x, y) || m_Bm.HasExplosionAt(x, y)) return -1;
    if (IsSpiritAt(x, y) || IsTurretAt(x, y)) return -1;
    return IsBlockedByOther(x, y) ? kOtherPlayerPenalty : 1;
}

// 炸牆開路：磚塊視為可走但成本高 (50)，傾向繞路、走投無路才炸
int BotNavigator::BrickCost(int x, int y) const {
    if (!m_Lm.IsWalkable(x, y) && !m_Lm.IsBrick(x, y)) return -1;
    if (m_Bm.IsBombAt(x, y) || m_Bm.HasExplosionAt(x, y) || m_Danger.IsLethal(x, y, m_Lm, m_Fp)) return -1;
    if (IsSpiritAt(x, y) || IsTurretAt(x, y)) return -1;
    if (m_Lm.IsBrick(x, y)) return 50;
    return IsBlockedByOther(x, y) ? kOtherPlayerPenalty : 1;
}

// 自殺攻擊：無視火焰與危險，只有牆與砲台擋路
int BotNavigator::SuicideCost(int x, int y) const {
    if (!m_Lm.IsWalkable(x, y) || IsTurretAt(x, y)) return -1;
    return 1;
}

// 在 (bx,by) 放 fp 火力的炸彈，火焰能否掃到 (tx,ty) (沿四方向、遇牆即止)
bool BotNavigator::BombReaches(int bx, int by, int tx, int ty) const {
    if (bx == tx && by == ty) return true;
    for (const auto& off : kCardinalOffsets) {
        for (int step = 1; step <= m_Fp; step++) {
            int cx = bx + off.dx * step;
            int cy = by + off.dy * step;
            if (cx == tx && cy == ty) return true;
            if (!m_Lm.IsWalkable(cx, cy)) break;
        }
    }
    return false;
}

bool BotNavigator::BombHitsAnySpirit(int bx, int by) const {
    for (const auto& s : m_Spirits) {
        if (s->ShouldDelete()) continue;
        if (BombReaches(bx, by, s->GetGridX(), s->GetGridY())) return true;
    }
    return false;
}

// 直線可視 (同行或同列、中間無牆) — 自殺攻擊判斷是否值得放彈
bool BotNavigator::HasLineOfSight(int bx, int by, int tx, int ty) const {
    if (bx != tx && by != ty) return false;
    int stepX = (tx > bx) ? 1 : (tx < bx) ? -1 : 0;
    int stepY = (ty > by) ? 1 : (ty < by) ? -1 : 0;
    int cx = bx + stepX, cy = by + stepY;
    while (cx != tx || cy != ty) {
        if (!m_Lm.IsWalkable(cx, cy)) return false;
        cx += stepX; cy += stepY;
    }
    return true;
}
