#ifndef KEYBINDINGS_HPP
#define KEYBINDINGS_HPP

#include "Controller/HumanController.hpp"  // Control
#include "Util/Keycode.hpp"

// 兩名人類玩家的按鍵設定 (可在設定畫面修改)。action 0..4 = 上/下/左/右/放炸彈。
// 未綁定的鍵以 NoKey() 表示 (Input 查詢不會命中 → 等同停用該動作)。
class KeyBindings {
public:
    static constexpr int kActions = 5;

    Control p[2] = {
        { Util::Keycode::W,  Util::Keycode::S,    Util::Keycode::A,    Util::Keycode::D,     Util::Keycode::SPACE },
        { Util::Keycode::UP, Util::Keycode::DOWN, Util::Keycode::LEFT, Util::Keycode::RIGHT, Util::Keycode::RSHIFT },
    };

    static Util::Keycode NoKey() { return static_cast<Util::Keycode>(0); }

    Util::Keycode& Key(int player, int action) {
        Control& c = p[player];
        switch (action) {
            case 0:  return c.UP;
            case 1:  return c.DOWN;
            case 2:  return c.LEFT;
            case 3:  return c.RIGHT;
            default: return c.PLACEBOMB;
        }
    }
    Util::Keycode Key(int player, int action) const {
        return const_cast<KeyBindings*>(this)->Key(player, action);
    }
};

#endif
