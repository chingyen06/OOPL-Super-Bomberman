#ifndef INPUT_CONTROLLER_HPP
#define INPUT_CONTROLLER_HPP

class InputController {
public:
    virtual ~InputController() = default;

    virtual bool IsUpPressed() const = 0;
    virtual bool IsDownPressed() const = 0;
    virtual bool IsLeftPressed() const = 0;
    virtual bool IsRightPressed() const = 0;
    virtual bool IsPlaceBombPressed() const = 0;
};

#endif
