#ifndef BOT_CONTROLLER_HPP
#define BOT_CONTROLLER_HPP

#include <memory>

#include "Bot/BotProfile.hpp"
#include "Controller/InputController.hpp"
#include "Controller/IProgrammableController.hpp"

// BotController 同時實作兩種介面：
//   - InputController         : 被 Player 當作輸入來源讀按鍵狀態
//   - IProgrammableController : 被 AIManager 寫入決策結果與冷卻
// AIManager 透過 IProgrammableController 操作 bot，不再 dynamic_cast 到具體類別。
// 每隻 bot 注入一個性格 (IBotProfile)，讓不同 bot 有不同想法。
class BotController : public InputController, public IProgrammableController {
public:
    // profile: 這隻 bot 的性格 (null 則退回預設)。phaseOffset: 出生時的初始 cooldown，
    // 不同 bot 給不同值可讓首次決策錯開。
    explicit BotController(std::shared_ptr<const IBotProfile> profile = nullptr, int phaseOffset = 0)
        : m_Profile(profile ? std::move(profile) : BotProfileFactory::Default()),
          m_DecisionCooldown(phaseOffset) {}

    // -------- InputController (讀) --------
    bool IsUpPressed() const override { return m_Up; }
    bool IsDownPressed() const override { return m_Down; }
    bool IsLeftPressed() const override { return m_Left; }
    bool IsRightPressed() const override { return m_Right; }
    bool IsPlaceBombJustPressed() const override { return m_PlaceBomb; }

    // -------- IProgrammableController (寫) --------
    void SetInput(bool up, bool down, bool left, bool right, bool placeBomb) override {
        m_Up = up;
        m_Down = down;
        m_Left = left;
        m_Right = right;
        m_PlaceBomb = placeBomb;
    }

    bool IsReadyToDecide() const override { return m_DecisionCooldown <= 0; }
    void TickCooldown() override { if (m_DecisionCooldown > 0) m_DecisionCooldown--; }
    void ResetCooldown(int frames) override { m_DecisionCooldown = frames; }

    const IBotProfile& Profile() const override { return *m_Profile; }

    void SetGoal(int gridX, int gridY) override { m_GoalX = gridX; m_GoalY = gridY; }
    int  GoalX() const override { return m_GoalX; }
    int  GoalY() const override { return m_GoalY; }

private:
    std::shared_ptr<const IBotProfile> m_Profile;  // 這隻 bot 的性格 (建構時注入)

    int m_GoalX = -1;  // 鎖定中的物件目標格 (anti-twitch；-1 = 無)
    int m_GoalY = -1;

    bool m_Up = false;
    bool m_Down = false;
    bool m_Left = false;
    bool m_Right = false;
    bool m_PlaceBomb = false;

    int m_DecisionCooldown = 0;  // 0 = 可決策；>0 = 還在冷卻
};

#endif
