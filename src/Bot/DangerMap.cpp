#include "DangerMap.hpp"

#include "BombManager.hpp"
#include "GameTypes.hpp"
#include "GridCoord.hpp"
#include "LevelManager.hpp"

#include <queue>

void DangerMap::Rebuild(const LevelManager& levelManager, const BombManager& bombManager) {
    const int mapW = levelManager.GetMapWidth();
    const int mapH = levelManager.GetMapHeight();
    m_Danger.assign(mapH, std::vector<bool>(mapW, false));

    // 1) 已存在的爆炸火焰
    for (int y = 0; y < mapH; ++y) {
        for (int x = 0; x < mapW; ++x) {
            if (bombManager.HasExplosionAt(x, y)) {
                m_Danger[y][x] = true;
            }
        }
    }

    // 2) 倒數中的炸彈會掃到的格子 (沿四方向延伸到第一個牆/磚為止)
    for (int by = 0; by < mapH; ++by) {
        for (int bx = 0; bx < mapW; ++bx) {
            if (!bombManager.IsBombAt(bx, by)) continue;

            const int bfp = bombManager.GetFirepowerAt(bx, by);
            m_Danger[by][bx] = true;

            for (const auto& off : kCardinalOffsets) {
                for (int step = 1; step <= bfp; ++step) {
                    const int cx = bx + off.dx * step;
                    const int cy = by + off.dy * step;
                    if (cx < 0 || cx >= mapW || cy < 0 || cy >= mapH) break;
                    m_Danger[cy][cx] = true;
                    if (!levelManager.IsWalkable(cx, cy)) break;
                }
            }
        }
    }
}

bool DangerMap::IsLethal(int tx, int ty, const LevelManager& levelManager, int fp,
                         int pretendX, int pretendY) const {
    // 對地圖外或越界格子保守視為不致命
    if (!GridCoord::InBounds(tx, ty)) return false;

    // 1) 真實炸彈/爆炸 — 直接查預先計算的快取
    if (m_Danger[ty][tx]) return true;

    // 2) 假想炸彈 (AI 想像在 pretendX/Y 放一顆 fp 火力的炸彈)
    if (pretendX >= 0 && pretendY >= 0) {
        if (tx == pretendX && ty == pretendY) return true;
        for (const auto& off : kCardinalOffsets) {
            for (int step = 1; step <= fp; ++step) {
                const int cx = pretendX + off.dx * step;
                const int cy = pretendY + off.dy * step;
                if (tx == cx && ty == cy) return true;
                if (!levelManager.IsWalkable(cx, cy)) break;
            }
        }
    }
    return false;
}

DangerMap::SafeSpot DangerMap::FindSafeSpot(int startX, int startY,
                                            const LevelManager& levelManager,
                                            const BombManager& bombManager,
                                            int botFp, int pretendX, int pretendY) const {
    SafeSpot bestSafe = { -1, -1, 999, false };
    const int mapW = levelManager.GetMapWidth();
    const int mapH = levelManager.GetMapHeight();

    std::vector<std::vector<bool>> visited(mapH, std::vector<bool>(mapW, false));
    std::queue<SafeSpot> q;
    q.push({ startX, startY, 0, false });
    visited[startY][startX] = true;

    while (!q.empty()) {
        const auto current = q.front();
        q.pop();

        if (!IsLethal(current.x, current.y, levelManager, botFp, pretendX, pretendY)) {
            bestSafe = { current.x, current.y, current.dist, true };
            break;
        }

        if (current.dist >= 5) continue;

        for (const auto& off : kCardinalOffsets) {
            const int nx = current.x + off.dx;
            const int ny = current.y + off.dy;

            if (nx >= 0 && nx < mapW && ny >= 0 && ny < mapH) {
                if (!visited[ny][nx] && levelManager.IsWalkable(nx, ny) &&
                    !bombManager.IsBombAt(nx, ny) && !bombManager.HasExplosionAt(nx, ny)) {
                    visited[ny][nx] = true;
                    q.push({ nx, ny, current.dist + 1, false });
                }
            }
        }
    }
    return bestSafe;
}

void DangerMap::RegisterPendingBomb(int bx, int by, int fp, const LevelManager& levelManager) {
    if (m_Danger.empty()) return;
    const int mapH = static_cast<int>(m_Danger.size());
    const int mapW = static_cast<int>(m_Danger[0].size());
    if (bx < 0 || by < 0 || bx >= mapW || by >= mapH) return;

    m_Danger[by][bx] = true;
    for (const auto& off : kCardinalOffsets) {
        for (int step = 1; step <= fp; ++step) {
            const int cx = bx + off.dx * step;
            const int cy = by + off.dy * step;
            if (cx < 0 || cy < 0 || cx >= mapW || cy >= mapH) break;
            m_Danger[cy][cx] = true;
            if (!levelManager.IsWalkable(cx, cy)) break;
        }
    }
}
