#ifndef MAPTILES_HPP
#define MAPTILES_HPP

#include "Util/GameObject.hpp"

class Tile : public Util::GameObject {
public:
    virtual bool IsPassable() const = 0;
    virtual bool IsDestructible() const { return false; }
};

// 草地
class Ground : public Tile {
public:
    Ground(int gridX, int gridY);
    bool IsPassable() const override {
        return true;
    };
};

// 無敵牆
class Wall : public Tile {
public:
    Wall(int gridX, int gridY);
    bool IsPassable() const override {
        return false;
    };
};

// 磚塊 (可破壞)
class Brick : public Tile {
public:
    Brick(int gridX, int gridY);
    bool IsPassable() const override {
        return false;
    };
    bool IsDestructible() const override {
        return true;
    }
};

#endif