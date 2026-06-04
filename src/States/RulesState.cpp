#include "States/RulesState.hpp"

#include <string>

#include "Core/App.hpp"
#include "States/BattleSetupState.hpp"
#include "States/MenuCommon.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"

// 「更換規則」可選的回合時間 (秒 / 顯示文字)
static const int   kTimeSecondsOptions[] = { 60, 120, 180, 300 };
static const char* kTimeLabels[]         = { "1 分", "2 分", "3 分", "5 分" };
static constexpr int kTimeOptionCount    = 4;

void RulesState::OnEnter(App& app) {
    app.ShowMenuBg();
    auto& root = app.Root();

    m_RowNormal = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/row_grey.png");
    m_RowSel    = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/row_sel.png");
    m_BtnNormal = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/btn.png");
    m_BtnSel    = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/btn_sel.png");

    m_Title = std::make_shared<UIText>("更換規則", -505.0f, 320.0f, 30.0f, MenuCommon::DarkText());
    root.AddChild(m_Title);

    // 固定顯示列 (不可選)
    m_FixedRow = std::make_shared<UIImage>(RESOURCE_DIR"/Image/row_grey.png", 0.0f, 200.0f, 20.0f);
    m_FixedLabel = std::make_shared<UIText>("對戰模式：城堡", kLabelXNudge, 200.0f - kLabelYNudge, 30.0f, MenuCommon::WhiteText());
    root.AddChild(m_FixedRow);
    root.AddChild(m_FixedLabel);

    // 3 個可調列
    const float ys[kRows] = { 130.0f, 70.0f, 10.0f };
    for (int i = 0; i < kRows; i++) {
        m_Rows[i] = std::make_shared<UIImage>(0.0f, ys[i], 20.0f);
        m_Rows[i]->SetDrawable(m_RowNormal);
        m_RowLabels[i] = std::make_shared<UIText>("-", kLabelXNudge, ys[i] - kLabelYNudge, 30.0f, MenuCommon::WhiteText());
        root.AddChild(m_Rows[i]);
        root.AddChild(m_RowLabels[i]);
    }

    // 確定鈕
    m_DoneBtn = std::make_shared<UIImage>(0.0f, -90.0f, 20.0f);
    m_DoneBtn->SetDrawable(m_BtnNormal);
    m_DoneLabel = std::make_shared<UIText>("確定", kLabelXNudge, -90.0f - kLabelYNudge, 30.0f, MenuCommon::DarkText());
    root.AddChild(m_DoneBtn);
    root.AddChild(m_DoneLabel);

    m_Sel = 0;
    Refresh(app);
    m_Hint = MenuCommon::AddKeyHint(app, {{"上下", "選擇"}, {"左右", "調整"}, {"空格鍵", "確定"}, {"X", "返回"}});
}

void RulesState::OnExit(App& app) {
    auto& root = app.Root();
    root.RemoveChild(m_Title);
    m_Hint.Remove(app);
    root.RemoveChild(m_FixedRow);
    root.RemoveChild(m_FixedLabel);
    root.RemoveChild(m_DoneBtn);
    root.RemoveChild(m_DoneLabel);
    for (int i = 0; i < kRows; i++) {
        root.RemoveChild(m_Rows[i]);
        root.RemoveChild(m_RowLabels[i]);
    }
    app.HideMenuBg();
}

int RulesState::TimeIndex(int seconds) const {
    for (int i = 0; i < kTimeOptionCount; i++) if (kTimeSecondsOptions[i] == seconds) return i;
    return 2;  // 預設 3 分
}

void RulesState::Adjust(MatchConfig& cfg, int dir) {
    if (m_Sel == 0) {
        const int ti = (TimeIndex(cfg.RoundSeconds()) + dir + kTimeOptionCount) % kTimeOptionCount;
        cfg.SetRoundSeconds(kTimeSecondsOptions[ti]);
    }
    else if (m_Sel == 1) cfg.SetSpiritsEnabled(!cfg.SpiritsEnabled());
    else if (m_Sel == 2) cfg.SetTurretsEnabled(!cfg.TurretsEnabled());
}

void RulesState::Refresh(App& app) {
    const MatchConfig& cfg = app.Session().Config();
    m_RowLabels[0]->SetText(std::string("時間：")    + kTimeLabels[TimeIndex(cfg.RoundSeconds())]);
    m_RowLabels[1]->SetText(std::string("源石精靈：") + (cfg.SpiritsEnabled() ? "開" : "關"));
    m_RowLabels[2]->SetText(std::string("砲台：")    + (cfg.TurretsEnabled() ? "開" : "關"));
    for (int i = 0; i < kRows; i++) m_Rows[i]->SetDrawable(m_Sel == i ? m_RowSel : m_RowNormal);
    const bool doneSel = (m_Sel == kRows);
    m_DoneBtn->SetDrawable(doneSel ? m_BtnSel : m_BtnNormal);
    m_DoneLabel->SetColor(doneSel ? MenuCommon::WhiteText() : MenuCommon::DarkText());
}

void RulesState::OnUpdate(App& app) {
    MatchConfig& cfg = app.Session().Config();
    bool dirty = false;
    if (Util::Input::IsKeyUp(Util::Keycode::UP) || Util::Input::IsKeyUp(Util::Keycode::W)) {
        m_Sel = (m_Sel + kItems - 1) % kItems; dirty = true;
    }
    else if (Util::Input::IsKeyUp(Util::Keycode::DOWN) || Util::Input::IsKeyUp(Util::Keycode::S)) {
        m_Sel = (m_Sel + 1) % kItems; dirty = true;
    }
    else if (Util::Input::IsKeyUp(Util::Keycode::LEFT) || Util::Input::IsKeyUp(Util::Keycode::A)) {
        Adjust(cfg, -1); dirty = true;
    }
    else if (Util::Input::IsKeyUp(Util::Keycode::RIGHT) || Util::Input::IsKeyUp(Util::Keycode::D)) {
        Adjust(cfg, +1); dirty = true;
    }
    if (dirty) Refresh(app);

    const bool confirm = (m_Sel == kRows) && Util::Input::IsKeyUp(Util::Keycode::SPACE);  // 確定
    if (confirm || Util::Input::IsKeyUp(Util::Keycode::X)) {  // X = 返回
        app.TransitionTo(std::make_unique<BattleSetupState>());
    }
}
