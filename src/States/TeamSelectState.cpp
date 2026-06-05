#include "States/TeamSelectState.hpp"

#include <string>

#include "Core/App.hpp"
#include "States/BattleSetupState.hpp"
#include "States/MenuCommon.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"

void TeamSelectState::OnEnter(App& app) {
    app.ShowMenuBg();
    auto& root = app.Root();

    m_RowDef    = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/row_def.png");
    m_RowAtk    = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/row_atk.png");
    m_RowGrey   = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/row_grey.png");
    m_BtnNormal = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/btn.png");
    m_BtnSel    = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/btn_sel.png");

    m_Title = std::make_shared<UIText>("選擇隊伍", -505.0f, 320.0f, 30.0f, MenuCommon::DarkText());
    root.AddChild(m_Title);

    for (int i = 0; i < kRows; i++) {  // row 0 = 玩家1；row 1..8 = 進攻席位 (slot 0..7)
        const float y = 230.0f - i * 48.0f;
        m_Rows[i] = std::make_shared<UIImage>(0.0f, y, 20.0f);
        m_Rows[i]->SetDrawable(m_RowGrey);
        m_RowLabels[i] = std::make_shared<UIText>("-", kLabelXNudge, y - kLabelYNudge, 30.0f, MenuCommon::WhiteText());
        root.AddChild(m_Rows[i]);
        root.AddChild(m_RowLabels[i]);
    }

    m_DoneBtn = std::make_shared<UIImage>(0.0f, -215.0f, 20.0f);
    m_DoneBtn->SetDrawable(m_BtnNormal);
    m_DoneLabel = std::make_shared<UIText>("確定", kLabelXNudge, -215.0f - kLabelYNudge, 30.0f, MenuCommon::DarkText());
    root.AddChild(m_DoneBtn);
    root.AddChild(m_DoneLabel);

    m_Sel = 0;
    Refresh(app);
    m_Hint = MenuCommon::AddKeyHint(app, {{"上下", "選擇"}, {"左右", "調整"}, {"空白鍵", "確定"}, {"X", "返回"}});
}

void TeamSelectState::OnExit(App& app) {
    auto& root = app.Root();
    root.RemoveChild(m_Title);
    m_Hint.Remove(app);
    root.RemoveChild(m_DoneBtn);
    root.RemoveChild(m_DoneLabel);
    for (int i = 0; i < kRows; i++) {
        root.RemoveChild(m_Rows[i]);
        root.RemoveChild(m_RowLabels[i]);
    }
    app.HideMenuBg();
}

void TeamSelectState::Adjust(MatchConfig& cfg, int dir) {
    if (m_Sel < 0 || m_Sel >= kAttackers) return;  // 只有席位列可調
    const MatchConfig::SlotMode cur = cfg.AttackerSlot(m_Sel);
    MatchConfig::SlotMode next;
    if (m_Sel == 0) {  // 玩家 2：循環 不加入 → 電腦 → 人類
        next = static_cast<MatchConfig::SlotMode>((static_cast<int>(cur) + dir + 3) % 3);
    }
    else {  // 電腦：切換 不加入 / 攻擊方
        next = (cur == MatchConfig::SlotMode::Off) ? MatchConfig::SlotMode::Computer
                                                   : MatchConfig::SlotMode::Off;
    }
    cfg.SetAttackerSlot(m_Sel, next);
    if (cfg.AttackerTotal() == 0) cfg.SetAttackerSlot(m_Sel, cur);  // 至少 1 攻擊方 → 還原
}

void TeamSelectState::Refresh(App& app) {
    const MatchConfig& cfg = app.Session().Config();

    // row 0：玩家 1 (固定守方)
    m_Rows[0]->SetDrawable(m_RowDef);
    m_RowLabels[0]->SetText("玩家 1　防守方");
    m_RowLabels[0]->SetColor(MenuCommon::WhiteText());

    // row 1..8：進攻席位
    for (int slot = 0; slot < kAttackers; slot++) {
        const int r = slot + 1;
        const MatchConfig::SlotMode m = cfg.AttackerSlot(slot);
        const std::string name = (slot == 0) ? "玩家 2" : ("電腦" + std::to_string(slot));
        const char* val = (m == MatchConfig::SlotMode::Off)   ? "不加入"
                        : (m == MatchConfig::SlotMode::Human)  ? "人類"
                        : (slot == 0)                          ? "電腦" : "攻擊方";
        m_Rows[r]->SetDrawable(m == MatchConfig::SlotMode::Off ? m_RowGrey : m_RowAtk);
        m_RowLabels[r]->SetText(name + "　" + val);
        m_RowLabels[r]->SetColor(m_Sel == slot ? MenuCommon::YellowText() : MenuCommon::WhiteText());  // 選取席位→黃字
    }

    const bool doneSel = (m_Sel == kDoneIndex);
    m_DoneBtn->SetDrawable(doneSel ? m_BtnSel : m_BtnNormal);
    m_DoneLabel->SetColor(doneSel ? MenuCommon::WhiteText() : MenuCommon::DarkText());
}

void TeamSelectState::OnUpdate(App& app) {
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

    const bool confirm = (m_Sel == kDoneIndex) && Util::Input::IsKeyUp(Util::Keycode::SPACE);  // 確定
    if (confirm || Util::Input::IsKeyUp(Util::Keycode::X)) {  // X = 返回
        app.TransitionTo(std::make_unique<BattleSetupState>());
    }
}
