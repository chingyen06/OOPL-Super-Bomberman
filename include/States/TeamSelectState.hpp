#ifndef TEAMSELECTSTATE_HPP
#define TEAMSELECTSTATE_HPP

#include <memory>

#include "Config/MatchConfig.hpp"
#include "States/IGameState.hpp"
#include "States/MenuCommon.hpp"
#include "UI/UIImage.hpp"
#include "UI/UIText.hpp"
#include "Util/Image.hpp"

// 選擇隊伍：玩家1 永遠是唯一守方；下方進攻席位可調 (玩家2: 不加入/電腦/人類；電腦席位: 攻擊方/不加入)。
class TeamSelectState : public IGameState {
public:
    void OnEnter(App& app) override;
    void OnExit(App& app) override;
    void OnUpdate(App& app) override;

private:
    static constexpr int kAttackers = MatchConfig::kMaxAttackers;  // 8 個進攻席位
    static constexpr int kRows      = kAttackers + 1;              // 玩家1 + 8 席位
    static constexpr int kItems     = kAttackers + 1;              // 8 席位 + 確定
    static constexpr int kDoneIndex = kAttackers;                  // 確定的 m_Sel 值

    void Adjust(MatchConfig& cfg, int dir);
    void Refresh(App& app);

    int m_Sel = 0;
    KeyHint m_Hint;
    std::shared_ptr<UIText>  m_Title, m_DoneLabel;
    std::shared_ptr<UIImage> m_DoneBtn;
    std::shared_ptr<Util::Image> m_RowDef, m_RowAtk, m_RowGrey, m_BtnNormal, m_BtnSel;
    std::shared_ptr<UIImage> m_Rows[kRows];
    std::shared_ptr<UIText>  m_RowLabels[kRows];
};

#endif
