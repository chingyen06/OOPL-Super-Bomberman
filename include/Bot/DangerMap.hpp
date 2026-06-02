#ifndef DANGERMAP_HPP
#define DANGERMAP_HPP

#include <vector>

class LevelManager;
class BombManager;

// 把「哪格會被火焰掃到」的計算與查詢從 AIManager 拆出，獨立成可測試、職責單一的元件。
// 對 AIManager 而言：每幀呼叫 Rebuild，之後用 IsLethal / FindSafeSpot 查詢；想評估「假設
// 我在 (px,py) 放一顆 fp 火力的炸彈」也透過介面 (pretendX/Y)，不必直接動內部資料。
class DangerMap {
public:
    // BFS 找到的安全格 (封裝；取代原本的公開資料 struct 式 class)
    class SafeSpot {
    public:
        SafeSpot(int x, int y, int dist, bool found)
            : m_X(x), m_Y(y), m_Dist(dist), m_Found(found) {}
        int  X() const { return m_X; }
        int  Y() const { return m_Y; }
        int  Dist() const { return m_Dist; }
        bool Found() const { return m_Found; }
    private:
        int  m_X;
        int  m_Y;
        int  m_Dist;
        bool m_Found;
    };

    // 重新計算危險地圖：包含 (1) 既有爆炸火焰 (2) 倒數中的炸彈未來爆炸範圍
    void Rebuild(const LevelManager& lm, const BombManager& bm);

    // 判斷 (tx, ty) 是否致命；可選擇加上「假想炸彈」(pretendX, pretendY, fp) 的影響
    bool IsLethal(int tx, int ty, const LevelManager& lm, int fp,
                  int pretendX = -1, int pretendY = -1) const;

    // 由 (startX, startY) 做 BFS 找最近的安全格 (深度上限 5 — 與原行為相同)
    SafeSpot FindSafeSpot(int startX, int startY,
                          const LevelManager& lm, const BombManager& bm,
                          int botFp, int pretendX = -1, int pretendY = -1) const;

    // 把「決策完還沒放下的 pending 炸彈」也算進 danger map，
    // 讓後續 bot 的 IsLethal/FindSafeSpot 自動納入考量 — 避免多 bot 同時放彈互炸。
    void RegisterPendingBomb(int bx, int by, int fp, const LevelManager& lm);

    // debug 視覺化用：直接查某格目前是否被標為危險 (Rebuild 之後)
    bool IsDanger(int x, int y) const {
        if (y < 0 || y >= static_cast<int>(m_Danger.size())) return false;
        if (x < 0 || x >= static_cast<int>(m_Danger[y].size())) return false;
        return m_Danger[y][x];
    }

private:
    std::vector<std::vector<bool>> m_Danger;  // [y][x] -> true 表示有炸彈火焰會掃到
};

#endif
