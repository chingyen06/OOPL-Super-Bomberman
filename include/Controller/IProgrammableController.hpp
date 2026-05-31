#ifndef IPROGRAMMABLE_CONTROLLER_HPP
#define IPROGRAMMABLE_CONTROLLER_HPP

#include "Bot/BotProfile.hpp"

// 與 InputController (純讀取介面) 分離開來：
// InputController 描述「任何能回答按鍵狀態的東西」(read-only)，
// IProgrammableController 描述「可由外部 (e.g. AIManager) 寫入按鍵與決策節奏的東西」。
//
// 拆兩個介面避免 HumanController 為了滿足 BotController 的需求而提供無意義的 stub
// (Interface Segregation Principle)。AIManager 對 BotController 的所有控制都透過
// 這個介面，不再 dynamic_cast 到具體類別 (改善 LSP)。
class IProgrammableController {
public:
    virtual ~IProgrammableController() = default;

    virtual void SetInput(bool up, bool down, bool left, bool right, bool placeBomb) = 0;

    // 反應延遲：bot 不每幀重新決策，模擬人類反應時間
    virtual bool IsReadyToDecide() const = 0;
    virtual void TickCooldown() = 0;
    virtual void ResetCooldown(int frames) = 0;

    // 這隻 bot 的性格 / 想法 (Strategy)。AIManager 依此調整反應速度、目標取向、膽量等。
    virtual const IBotProfile& Profile() const = 0;

    // 鎖定中的物件目標格 (-1 表示無)。讓 bot 對選定目標「有承諾」、不每幀重挑最近物件，
    // 避免多隻 bot 互搶鄰近鑰匙時目標反覆互換而原地左右抽搐。
    virtual void SetGoal(int gridX, int gridY) = 0;
    virtual int  GoalX() const = 0;
    virtual int  GoalY() const = 0;
};

#endif
