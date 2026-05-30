#ifndef IGAMESTATE_HPP
#define IGAMESTATE_HPP

class App;

// 遊戲狀態介面 (狀態模式)：各畫面/階段以子類別實作，App 持有目前狀態。
class IGameState {
public:
    virtual ~IGameState() = default;
    virtual void OnEnter(App& /*app*/) {}
    virtual void OnUpdate(App& app) = 0;
    virtual void OnExit(App& /*app*/) {}
    // 是否顯示滑鼠：預設隱藏 (遊戲/選單)，僅暫停與 debug 主控台需要
    virtual bool WantsCursor(App& /*app*/) { return false; }
};

#endif
