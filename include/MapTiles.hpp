#ifndef MAPTILES_HPP
#define MAPTILES_HPP

#include "Util/GameObject.hpp"

// 草地
class Ground : public Util::GameObject {
public:
    Ground(int gridX, int gridY);
};

// 無敵牆
class Wall : public Util::GameObject {
public:
    Wall(int gridX, int gridY);
};

// 磚塊 (可破壞)
class Brick : public Util::GameObject {
public:
    Brick(int gridX, int gridY);
};

#endif