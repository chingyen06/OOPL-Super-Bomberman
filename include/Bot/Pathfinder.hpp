#ifndef PATHFINDER_HPP
#define PATHFINDER_HPP

#include <functional>
#include <memory>
#include <utility>
#include <vector>

// A* 尋路節點 (封裝)。
class AStarNode {
public:
    AStarNode(int x, int y, int g, int h, std::shared_ptr<AStarNode> parent)
        : m_X(x), m_Y(y), m_G(g), m_H(h), m_Parent(std::move(parent)) {}

    int X() const { return m_X; }
    int Y() const { return m_Y; }
    int G() const { return m_G; }
    int F() const { return m_G + m_H; }  // f = g + h
    const std::shared_ptr<AStarNode>& Parent() const { return m_Parent; }

private:
    int m_X;
    int m_Y;
    int m_G;
    int m_H;
    std::shared_ptr<AStarNode> m_Parent;
};

// priority_queue 比較器：f 值小者優先 (min-heap)。
class CompareNode {
public:
    bool operator()(const std::shared_ptr<AStarNode>& a, const std::shared_ptr<AStarNode>& b) const {
        return a->F() > b->F();
    }
};

// 泛用 A* 尋路 (自 AIManager 抽出，SRP)。走法經 costFunc 注入 (<0 不可走，>=0 為進入成本)；
// 回傳路徑不含起點。
class Pathfinder {
public:
    std::vector<std::pair<int, int>> FindPath(int startX, int startY, int targetX, int targetY,
                                              int mapW, int mapH,
                                              const std::function<int(int, int)>& costFunc) const;
};

#endif
