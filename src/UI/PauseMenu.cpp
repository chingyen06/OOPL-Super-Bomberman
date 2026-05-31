#include "PauseMenu.hpp"

#include "Util/Color.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"

void PauseMenu::Init() {
    // 預載按鈕底圖 (一般 / 選取)，之後切換選取狀態只換 drawable，不重讀檔
    m_BtnNormal   = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/pause_btn.png");
    m_BtnSelected = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/pause_btn_sel.png");

    // 全螢幕變暗 (半透明黑) — 蓋在凍結的遊戲畫面上
    m_Dim = std::make_shared<UIImage>(RESOURCE_DIR"/Image/pause_dim.png", 0.0f, 0.0f, kDimZ);
    m_Dim->SetFullScreen();

    // 右側面板 (稍深的半透明) — 原圖 16x16，放大成 360x720
    m_Panel = std::make_shared<UIImage>(RESOURCE_DIR"/Image/pause_panel.png", kPanelCenterX, 0.0f, kPanelZ);
    m_Panel->SetScale(360.0f / 16.0f, 720.0f / 16.0f);

    m_Title = std::make_shared<UIText>("暫停", kPanelCenterX, kTitleY, kTextZ,
                                       Util::Color::FromName(Util::Colors::WHITE));
}

void PauseMenu::AddOption(const std::string& text, std::function<void()> onSelect) {
    const float y = kFirstOptionY - static_cast<float>(m_RawTexts.size()) * kOptionStepY;

    auto button = std::make_shared<UIImage>(RESOURCE_DIR"/Image/pause_btn.png", kPanelCenterX, y, kButtonZ);
    // 文字略往右一點 (與其他按鈕一致)
    auto label  = std::make_shared<UIText>(text, kPanelCenterX + 5.0f, y, kTextZ,
                                           Util::Color::FromName(Util::Colors::WHITE));

    m_RawTexts.push_back(text);
    m_Buttons.push_back(button);
    m_Labels.push_back(label);
    m_Callbacks.push_back(onSelect);
    m_Enabled.push_back(true);
}

void PauseMenu::SetOptionLabel(int index, const std::string& text) {
    if (index < 0 || index >= static_cast<int>(m_Labels.size())) return;
    m_RawTexts[index] = text;
    m_Labels[index]->SetText(text);
}

void PauseMenu::SetOptionEnabled(int index, bool enabled) {
    if (index < 0 || index >= static_cast<int>(m_Enabled.size())) return;
    m_Enabled[index] = enabled;
    if (m_IsVisible) {
        if (!m_Enabled[m_SelectedIndex]) m_SelectedIndex = StepEnabled(m_SelectedIndex, +1);
        UpdateCursor();
    }
}

int PauseMenu::StepEnabled(int from, int dir) const {
    const int n = static_cast<int>(m_Buttons.size());
    if (n == 0) return 0;
    int idx = from;
    for (int i = 0; i < n; i++) {
        idx = (idx + dir + n) % n;
        if (m_Enabled[idx]) return idx;
    }
    return (from < 0) ? 0 : from;  // 全部停用：維持原樣
}

void PauseMenu::Show(Util::Renderer& root) {
    if (m_IsVisible) return;
    m_IsVisible = true;
    m_SelectedIndex = StepEnabled(-1, +1);  // 落在第一個可選項

    root.AddChild(m_Dim);
    root.AddChild(m_Panel);
    root.AddChild(m_Title);
    for (auto& button : m_Buttons) root.AddChild(button);
    for (auto& label : m_Labels)   root.AddChild(label);

    UpdateCursor();
}

void PauseMenu::Hide(Util::Renderer& root) {
    if (!m_IsVisible) return;
    m_IsVisible = false;

    root.RemoveChild(m_Dim);
    root.RemoveChild(m_Panel);
    root.RemoveChild(m_Title);
    for (auto& button : m_Buttons) root.RemoveChild(button);
    for (auto& label : m_Labels)   root.RemoveChild(label);
}

void PauseMenu::Update() {
    if (!m_IsVisible || m_Buttons.empty()) return;

    const bool prev = Util::Input::IsKeyUp(Util::Keycode::UP)   || Util::Input::IsKeyUp(Util::Keycode::LEFT)  ||
                      Util::Input::IsKeyUp(Util::Keycode::W)    || Util::Input::IsKeyUp(Util::Keycode::A);
    const bool next = Util::Input::IsKeyUp(Util::Keycode::DOWN) || Util::Input::IsKeyUp(Util::Keycode::RIGHT) ||
                      Util::Input::IsKeyUp(Util::Keycode::S)    || Util::Input::IsKeyUp(Util::Keycode::D);

    bool changed = false;
    if (prev)      { m_SelectedIndex = StepEnabled(m_SelectedIndex, -1); changed = true; }
    else if (next) { m_SelectedIndex = StepEnabled(m_SelectedIndex, +1); changed = true; }

    if (changed) UpdateCursor();

    // SPACE = 確認選取的選項 (ENTER 由 GamePausedState 攔截作為「退回去」)。
    if (Util::Input::IsKeyUp(Util::Keycode::SPACE)) {
        // 注意：callback 可能會 TransitionTo 並隱藏本選單，呼叫後立即 return，
        // 不再觸碰任何成員 (與專案其餘 state 轉移慣例一致)。
        if (m_Enabled[m_SelectedIndex] && m_Callbacks[m_SelectedIndex]) m_Callbacks[m_SelectedIndex]();
        return;
    }
}

void PauseMenu::UpdateCursor() {
    for (size_t i = 0; i < m_Buttons.size(); i++) {
        const bool sel = (static_cast<int>(i) == m_SelectedIndex);
        m_Buttons[i]->SetDrawable(sel ? m_BtnSelected : m_BtnNormal);
        // 停用項變灰，其餘白字
        m_Labels[i]->SetColor(m_Enabled[i] ? Util::Color::FromName(Util::Colors::WHITE)
                                           : Util::Color::FromName(Util::Colors::GRAY));
    }
}
