#ifndef MAINMENUSTATE_HPP
#define MAINMENUSTATE_HPP

#include <memory>

#include "States/IGameState.hpp"
#include "States/MenuCommon.hpp"
#include "UI/UIButtonList.hpp"
#include "UI/UIImage.hpp"
#include "UI/UIText.hpp"

// 主選單：開始對戰 / 離開遊戲；設定改由右下角齒輪進入 (按 → 選取、空白鍵進入)。
class MainMenuState : public IGameState {
public:
    void OnEnter(App& app) override;
    void OnExit(App& app) override;
    void OnUpdate(App& app) override;

private:
    void UpdateGearVisual();

    UIButtonList m_Menu;
    std::shared_ptr<UIText>  m_Title;
    std::shared_ptr<UIText>  m_Subtitle;
    std::shared_ptr<UIImage> m_Gear;
    bool m_GearFocused = false;  // true = 焦點在右下角齒輪上
    KeyHint m_Hint;
    CoinHud m_Coins;
};

#endif
