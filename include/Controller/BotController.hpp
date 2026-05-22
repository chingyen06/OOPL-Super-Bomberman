#ifndef BOT_CONTROLLER_HPP
#define BOT_CONTROLLER_HPP

#include "Controller/InputController.hpp"

class BotController : public InputController {
public:
    // phaseOffset: 出生時的初始 cooldown，不同 bot 給不同值可讓首次決策錯開
    explicit BotController(int phaseOffset = 0) : m_DecisionCooldown(phaseOffset) {}

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

    // 反應延遲：bot 不每幀重新決策，模擬人類反應時間
    bool IsReadyToDecide() const { return m_DecisionCooldown <= 0; }
    void TickCooldown() { if (m_DecisionCooldown > 0) m_DecisionCooldown--; }
    void ResetCooldown(int frames) { m_DecisionCooldown = frames; }

private:
    bool m_Up = false;
    bool m_Down = false;
    bool m_Left = false;
    bool m_Right = false;
    bool m_PlaceBomb = false;

    int m_DecisionCooldown = 0;  // 0 = 可決策；>0 = 還在冷卻
};

#endif
