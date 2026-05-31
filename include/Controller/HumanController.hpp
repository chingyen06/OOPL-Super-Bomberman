#ifndef HUMAN_CONTROLLER_HPP
#define HUMAN_CONTROLLER_HPP

#include "Controller/InputController.hpp"
#include "Util/Keycode.hpp"
#include "Util/Input.hpp"

class Control {
public:
    Util::Keycode UP;
    Util::Keycode DOWN;
    Util::Keycode LEFT;
    Util::Keycode RIGHT;
    Util::Keycode PLACEBOMB;
    Util::Keycode WEAPON;  // 防守方武器發動
};

class HumanController : public InputController {
public:
    HumanController(Control control) : m_Control(control) {}

    bool IsUpPressed() const override { return Util::Input::IsKeyPressed(m_Control.UP); }
    bool IsDownPressed() const override { return Util::Input::IsKeyPressed(m_Control.DOWN); }
    bool IsLeftPressed() const override { return Util::Input::IsKeyPressed(m_Control.LEFT); }
    bool IsRightPressed() const override { return Util::Input::IsKeyPressed(m_Control.RIGHT); }
    bool IsPlaceBombJustPressed() const override { return Util::Input::IsKeyDown(m_Control.PLACEBOMB); }
    bool IsWeaponJustPressed() const override { return Util::Input::IsKeyDown(m_Control.WEAPON); }

    Util::Keycode GetBombKey() const { return m_Control.PLACEBOMB; }

private:
    Control m_Control;
};

#endif
