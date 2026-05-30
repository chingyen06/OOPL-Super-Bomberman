#ifndef RULESSTATE_HPP
#define RULESSTATE_HPP

#include <memory>

#include "Config/MatchConfig.hpp"
#include "States/IGameState.hpp"
#include "States/MenuCommon.hpp"
#include "UI/UIImage.hpp"
#include "UI/UIText.hpp"
#include "Util/Image.hpp"

// 更換規則：上下選擇、左右調整 (時間 / 源石精靈 / 砲台)，直接寫入 GameSession 的 MatchConfig。
class RulesState : public IGameState {
public:
    void OnEnter(App& app) override;
    void OnExit(App& app) override;
    void OnUpdate(App& app) override;

private:
    static constexpr int kRows  = 3;  // 時間 / 源石精靈 / 砲台
    static constexpr int kItems = 4;  // 上面 3 列 + 確定

    int  TimeIndex(int seconds) const;
    void Adjust(MatchConfig& cfg, int dir);
    void Refresh(App& app);

    int m_Sel = 0;
    KeyHint m_Hint;
    std::shared_ptr<UIText>  m_Title, m_FixedLabel, m_DoneLabel;
    std::shared_ptr<UIImage> m_FixedRow, m_DoneBtn;
    std::shared_ptr<Util::Image> m_RowNormal, m_RowSel, m_BtnNormal, m_BtnSel;
    std::shared_ptr<UIImage> m_Rows[kRows];
    std::shared_ptr<UIText>  m_RowLabels[kRows];
};

#endif
