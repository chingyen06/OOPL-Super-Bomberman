#ifndef LEVELSELECTSTATE_HPP
#define LEVELSELECTSTATE_HPP

#include <memory>

#include "States/IGameState.hpp"
#include "States/MenuCommon.hpp"
#include "UI/UIButtonList.hpp"
#include "UI/UIText.hpp"

// 選擇關卡：縮圖列表，左右選擇、空白鍵確定。
class LevelSelectState : public IGameState {
public:
    void OnEnter(App& app) override;
    void OnExit(App& app) override;
    void OnUpdate(App& app) override;

private:
    UIButtonList m_Thumbs;
    std::shared_ptr<UIText> m_Title;
    KeyHint m_Hint;
};

#endif
