#ifndef HUMAN_CONTROLLER_HPP
#define HUMAN_CONTROLLER_HPP

#include "Controller/InputController.hpp"
#include "Util/Keycode.hpp"
#include "Util/Input.hpp"

// 一名玩家的按鍵設定。成員維持公開：KeyBindings::Key() 直接回傳其 reference 供設定畫面就地改鍵。
class Control {
public:
    Control(Util::Keycode up, Util::Keycode down, Util::Keycode left, Util::Keycode right,
            Util::Keycode placeBomb, Util::Keycode weapon)
        : UP(up), DOWN(down), LEFT(left), RIGHT(right), PLACEBOMB(placeBomb), WEAPON(weapon) {}

    Util::Keycode UP, DOWN, LEFT, RIGHT, PLACEBOMB;
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
