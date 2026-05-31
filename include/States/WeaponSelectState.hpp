#ifndef WEAPONSELECTSTATE_HPP
#define WEAPONSELECTSTATE_HPP

#include <memory>

#include "States/IGameState.hpp"
#include "States/MenuCommon.hpp"
#include "UI/UIButtonList.hpp"
#include "UI/UIText.hpp"

// 選擇武器：城堡模式開局前，防守方三選一 (劍 / 雷射砲 / 屏障)。寫入 MatchConfig。
class WeaponSelectState : public IGameState {
public:
    void OnEnter(App& app) override;
    void OnExit(App& app) override;
    void OnUpdate(App& app) override;

private:
    void RefreshDesc();

    UIButtonList m_List;
    std::shared_ptr<UIText> m_Title;
    std::shared_ptr<UIText> m_Desc;
    KeyHint m_Hint;
};

#endif
