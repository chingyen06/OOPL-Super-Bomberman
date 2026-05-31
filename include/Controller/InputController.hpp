#ifndef INPUT_CONTROLLER_HPP
#define INPUT_CONTROLLER_HPP

class InputController {
public:
    virtual ~InputController() = default;

    virtual bool IsUpPressed() const = 0;
    virtual bool IsDownPressed() const = 0;
    virtual bool IsLeftPressed() const = 0;
    virtual bool IsRightPressed() const = 0;
    // 注意：這是邊緣觸發 (key-down 那一幀才回 true)，不是長按
    virtual bool IsPlaceBombJustPressed() const = 0;

    // 防守方武器發動 (邊緣觸發)。預設 false：非人類/無武器者不需覆寫 (ISP)。
    virtual bool IsWeaponJustPressed() const { return false; }
};

#endif
