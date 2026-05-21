#ifndef BOT_CONTROLLER_HPP
#define BOT_CONTROLLER_HPP

#include "Controller/InputController.hpp"

class BotController : public InputController {
public:
    BotController() = default;

    bool IsUpPressed() const override { return m_Up; }
    bool IsDownPressed() const override { return m_Down; }
    bool IsLeftPressed() const override { return m_Left; }
    bool IsRightPressed() const override { return m_Right; }
    bool IsPlaceBombJustPressed() const override { return m_PlaceBomb; }

    void SetInput(bool up, bool down, bool left, bool right, bool placeBomb) {
        m_Up = up;
        m_Down = down;
        m_Left = left;
        m_Right = right;
        m_PlaceBomb = placeBomb;
    }

private:
    bool m_Up = false;
    bool m_Down = false;
    bool m_Left = false;
    bool m_Right = false;
    bool m_PlaceBomb = false;
};

#endif
