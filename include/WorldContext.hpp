#ifndef WORLDCONTEXT_HPP
#define WORLDCONTEXT_HPP

#include "glm/vec2.hpp"

// Player 移動/碰撞所需的世界查詢 (最小視角)。
class IWorldContext {
public:
    virtual ~IWorldContext() = default;

    virtual bool IsWalkable(int gridX, int gridY) const = 0;
    virtual bool IsBombAt(int gridX, int gridY) const = 0;
    virtual bool IsTurretAt(int gridX, int gridY) const = 0;
    virtual glm::vec2 GetForceAt(int gridX, int gridY) const = 0;
};

// 敵方 (Spirit / 未來的 AI) 需要的較豐富視角：在移動查詢之外，額外暴露「該格是否有爆炸火焰」。
// Player 不需要這個 (玩家燒死由 BombManager 以像素 AABB 判定)，故以介面繼承做 ISP 切分 —
// Player 仍只依賴較窄的 IWorldContext，不被迫看到用不到的查詢。
class IEnemyWorldContext : public IWorldContext {
public:
    virtual bool HasExplosionAt(int gridX, int gridY) const = 0;
};

#endif
