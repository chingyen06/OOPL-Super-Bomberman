#ifndef COOLDOWN_RESET_GUARD_HPP
#define COOLDOWN_RESET_GUARD_HPP

#include "Controller/IProgrammableController.hpp"

// RAII 守衛：離開作用域時把指定 controller 的決策冷卻設為 frames。
// 抽自 AIManager::Update 內原本的函式區域類別 (改放 header — class 不寫在 .cpp)。
// 不可複製：避免被複製後在多個作用域結束時重複設定冷卻。
class CooldownResetGuard {
public:
    CooldownResetGuard(IProgrammableController* ctrl, int frames)
        : m_Ctrl(ctrl), m_Frames(frames) {}
    ~CooldownResetGuard() { m_Ctrl->ResetCooldown(m_Frames); }

    CooldownResetGuard(const CooldownResetGuard&)            = delete;
    CooldownResetGuard& operator=(const CooldownResetGuard&) = delete;

private:
    IProgrammableController* m_Ctrl;
    int m_Frames;
};

#endif
