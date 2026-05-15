#ifndef WORLDCONTEXT_HPP
#define WORLDCONTEXT_HPP

#include "glm/vec2.hpp"

class IWorldContext {
public:
    virtual ~IWorldContext() = default;

    virtual bool IsWalkable(int gridX, int gridY) const = 0;
    virtual bool IsBombAt(int gridX, int gridY) const = 0;
    virtual bool IsTurretAt(int gridX, int gridY) const = 0;
    virtual glm::vec2 GetForceAt(int gridX, int gridY) const = 0;
};

#endif
