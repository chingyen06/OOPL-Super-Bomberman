#ifndef BOMB_HPP
#define BOMB_HPP

#include "Util/GameObject.hpp"
#include "Util/Image.hpp"
#include <memory>

class Bomb : public Util::GameObject {
public:
    enum class State { COUNTDOWN, DONE };

    Bomb(int gridX, int gridY, int firepower);

    void Update();

    State GetState() const { return m_State; }
    int GetGridX() const { return m_GridX; }
    int GetGridY() const { return m_GridY; }
    int GetFirepower() const { return m_Firepower; }

private:
    int m_GridX;
    int m_GridY;
    int m_Firepower;
    int m_Tick;
    State m_State;
};

#endif