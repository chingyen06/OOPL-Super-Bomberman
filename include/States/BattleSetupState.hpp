#ifndef BATTLESETUPSTATE_HPP
#define BATTLESETUPSTATE_HPP

#include <memory>
#include <vector>

#include "States/IGameState.hpp"
#include "States/MenuCommon.hpp"
#include "UI/UIButtonList.hpp"
#include "UI/UIImage.hpp"
#include "UI/UIText.hpp"

// 對戰設定 (離線戰鬥)：左側導覽 (對戰開始 / 更換規則 / 選擇隊伍 / 選擇關卡) + 成員格顯示。
class BattleSetupState : public IGameState {
public:
    void OnEnter(App& app) override;
    void OnExit(App& app) override;
    void OnUpdate(App& app) override;

private:
    UIButtonList m_Nav;
    std::shared_ptr<UIText> m_Title;
    std::shared_ptr<UIText> m_LevelInfo;
    KeyHint m_Hint;
    CoinHud m_Coins;
    std::vector<std::shared_ptr<UIImage>> m_Slots;
    std::vector<std::shared_ptr<UIText>>  m_SlotLabels;
};

#endif
