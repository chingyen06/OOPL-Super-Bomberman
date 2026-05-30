#ifndef TITLESCREENSTATE_HPP
#define TITLESCREENSTATE_HPP

#include <memory>

#include "States/IGameState.hpp"
#include "UI/UIImage.hpp"
#include "UI/UIText.hpp"

// 封面：去字的彩屑背景 + 整體上下彈跳的 logo + 常駐「請按空白鍵」。
class TitleScreenState : public IGameState {
public:
    void OnEnter(App& app) override;
    void OnExit(App& app) override;
    void OnUpdate(App& app) override;

private:
    static constexpr float kLogoBaseY = 45.0f;  // logo 靜止中心 y

    int m_Tick = 0;
    std::shared_ptr<UIImage> m_Logo;
    std::shared_ptr<UIText>  m_Press;
};

#endif
