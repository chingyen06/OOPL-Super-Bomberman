#ifndef KEYBINDINGS_HPP
#define KEYBINDINGS_HPP

#include "Controller/HumanController.hpp"  // Control
#include "Util/Keycode.hpp"

// 兩名人類玩家的按鍵設定 (可在設定畫面修改)。action 0..4 = 上/下/左/右/放炸彈。
// 未綁定的鍵以 NoKey() 表示 (Input 查詢不會命中 → 等同停用該動作)。
class KeyBindings {
public:
    static constexpr int kActions = 6;  // 上/下/左/右/放炸彈/武器發動

    Control p[2] = {
        { Util::Keycode::W,  Util::Keycode::S,    Util::Keycode::A,    Util::Keycode::D,     Util::Keycode::SPACE,  Util::Keycode::E },
        { Util::Keycode::UP, Util::Keycode::DOWN, Util::Keycode::LEFT, Util::Keycode::RIGHT, Util::Keycode::RSHIFT, static_cast<Util::Keycode>(0) },
    };

    // 暫停：兩名玩家共用的單一按鍵 (可在設定畫面修改)，預設 Enter。
    Util::Keycode pause = Util::Keycode::RETURN;

    static Util::Keycode NoKey() { return static_cast<Util::Keycode>(0); }

    Util::Keycode& Key(int player, int action) {
        Control& c = p[player];
        switch (action) {
            case 0:  return c.UP;
            case 1:  return c.DOWN;
            case 2:  return c.LEFT;
            case 3:  return c.RIGHT;
            case 4:  return c.PLACEBOMB;
            default: return c.WEAPON;
        }
    }
    Util::Keycode Key(int player, int action) const {
        return const_cast<KeyBindings*>(this)->Key(player, action);
    }
};

#endif
