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
    auto label  = std::make_shared<UIText>(text, kPanelCenterX, y, kTextZ,
                                           Util::Color::FromName(Util::Colors::WHITE));

    m_RawTexts.push_back(text);
    m_Buttons.push_back(button);
    m_Labels.push_back(label);
    m_Callbacks.push_back(onSelect);
}

void PauseMenu::SetOptionLabel(int index, const std::string& text) {
    if (index < 0 || index >= static_cast<int>(m_Labels.size())) return;
    m_RawTexts[index] = text;
    m_Labels[index]->SetText(text);
}

void PauseMenu::Show(Util::Renderer& root) {
    if (m_IsVisible) return;
    m_IsVisible = true;
    m_SelectedIndex = 0;

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
    if (prev) {
        m_SelectedIndex--;
        if (m_SelectedIndex < 0) m_SelectedIndex = static_cast<int>(m_Buttons.size()) - 1;
        changed = true;
    }
    else if (next) {
        m_SelectedIndex++;
        if (m_SelectedIndex >= static_cast<int>(m_Buttons.size())) m_SelectedIndex = 0;
        changed = true;
    }

    if (changed) UpdateCursor();

    // SPACE = 確認選取的選項 (ENTER 由 GamePausedState 攔截作為「退回去」)。
    if (Util::Input::IsKeyUp(Util::Keycode::SPACE)) {
        // 注意：callback 可能會 TransitionTo 並隱藏本選單，呼叫後立即 return，
        // 不再觸碰任何成員 (與專案其餘 state 轉移慣例一致)。
        if (m_Callbacks[m_SelectedIndex]) m_Callbacks[m_SelectedIndex]();
        return;
    }
}

void PauseMenu::UpdateCursor() {
    for (size_t i = 0; i < m_Buttons.size(); i++) {
        m_Buttons[i]->SetDrawable(static_cast<int>(i) == m_SelectedIndex ? m_BtnSelected : m_BtnNormal);
    }
}
