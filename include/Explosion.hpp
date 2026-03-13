#ifndef EXPLOSION_HPP
#define EXPLOSION_HPP

#include "Util/GameObject.hpp"
#include "Util/Image.hpp"
#include <memory>

class Explosion : public Util::GameObject {
public:
    Explosion(int gridX, int gridY);
    void Update();

    bool IsDone() const { return m_Done; }
    int GetGridX() const { return m_GridX; }
    int GetGridY() const { return m_GridY; }

private:
    int m_GridX;
    int m_GridY;
    int m_Tick;
    bool m_Done;
};

#endif