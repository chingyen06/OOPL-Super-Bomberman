#ifndef HUMAN_CONTROLLER_HPP
#define HUMAN_CONTROLLER_HPP

#include "Controller/InputController.hpp"
#include "Util/Keycode.hpp"
#include "Util/Input.hpp"

#include <SDL_keyboard.h>   // SDL_GetKeyboardState
#include <SDL_scancode.h>   // SDL_Scancode / SDL_NUM_SCANCODES

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

    // 移動（長按）：直接讀 SDL 即時鍵盤狀態，不走 PTSD 的事件式 s_KeyState。
    // 因為 PTSD 的 Input::Update() 在 ImGui 抓滑鼠 (WantCaptureMouse，例如開 F3 debug 主控台) 時
    // 會連鍵盤事件一起略過，導致放開的方向鍵 KEYUP 遺失、按住狀態卡住，關閉 debug 後人物像被
    // 某方向卡死、要再按一下方向鍵才解除。SDL_GetKeyboardState 反映實際物理按鍵，不受此影響。
    bool IsUpPressed() const override { return KeyHeld(m_Control.UP); }
    bool IsDownPressed() const override { return KeyHeld(m_Control.DOWN); }
    bool IsLeftPressed() const override { return KeyHeld(m_Control.LEFT); }
    bool IsRightPressed() const override { return KeyHeld(m_Control.RIGHT); }
    // 放炸彈 / 武器為邊緣觸發 (按下那一幀)，沿用 PTSD 的 IsKeyDown。
    bool IsPlaceBombJustPressed() const override { return Util::Input::IsKeyDown(m_Control.PLACEBOMB); }
    bool IsWeaponJustPressed() const override { return Util::Input::IsKeyDown(m_Control.WEAPON); }

    Util::Keycode GetBombKey() const { return m_Control.PLACEBOMB; }

private:
    // PTSD 的鍵盤 Keycode 即 SDL scancode (見 Input::UpdateKeyState)，可直接索引 SDL 鍵盤狀態表。
    static bool KeyHeld(Util::Keycode key) {
        const Uint8* ks = SDL_GetKeyboardState(nullptr);
        const auto sc = static_cast<int>(key);
        return sc >= 0 && sc < SDL_NUM_SCANCODES && ks[sc] != 0;
    }

    Control m_Control;
};

#endif
