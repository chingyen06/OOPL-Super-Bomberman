#include "UI/UIButtonList.hpp"

#include "Util/Color.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"

void UIButtonList::Init(const std::string& normalImg, const std::string& selectedImg) {
    m_NormalImg   = std::make_shared<Util::Image>(normalImg);
    m_SelectedImg = std::make_shared<Util::Image>(selectedImg);
}

void UIButtonList::AddItem(const std::string& text) {
    auto button = std::make_shared<UIImage>(0.0f, 0.0f, kButtonZ);
    button->SetDrawable(m_NormalImg);
    auto label = std::make_shared<UIText>(text, 0.0f, 0.0f, kLabelZ,
                                          Util::Color::FromName(Util::Colors::BLACK));

    m_Texts.push_back(text);
    m_Buttons.push_back(button);
    m_Labels.push_back(label);
    m_Enabled.push_back(true);
}

void UIButtonList::SetItemLabel(int index, const std::string& text) {
    if (index < 0 || index >= static_cast<int>(m_Labels.size())) return;
    m_Texts[index] = text;
    m_Labels[index]->SetText(text);
}

void UIButtonList::SetItemEnabled(int index, bool enabled) {
    if (index < 0 || index >= static_cast<int>(m_Enabled.size())) return;
    m_Enabled[index] = enabled;
}

void UIButtonList::SetSelected(int index) {
    if (index < 0 || index >= static_cast<int>(m_Buttons.size())) return;
    if (!m_Enabled[index]) return;
    m_Selected = index;
    if (m_Visible) UpdateCursor();
}

void UIButtonList::SetHighlight(bool on) {
    if (m_Highlight == on) return;
    m_Highlight = on;
    if (m_Visible) UpdateCursor();
}

void UIButtonList::Show(Util::Renderer& root, float startX, float startY, float stepX, float stepY) {
    if (m_Visible) return;
    m_Visible = true;

    for (size_t i = 0; i < m_Buttons.size(); i++) {
        const float x = startX + static_cast<float>(i) * stepX;
        const float y = startY + static_cast<float>(i) * stepY;
        m_Buttons[i]->SetPosition(x, y);
        m_Labels[i]->SetPosition(x + kLabelXNudge, y - kLabelYNudge);
        root.AddChild(m_Buttons[i]);
        root.AddChild(m_Labels[i]);
    }

    m_Selected = StepToEnabled(-1, +1);  // 第一個可選項
    UpdateCursor();
}

void UIButtonList::Hide(Util::Renderer& root) {
    if (!m_Visible) return;
    m_Visible = false;
    for (auto& button : m_Buttons) root.RemoveChild(button);
    for (auto& label : m_Labels)   root.RemoveChild(label);
}

int UIButtonList::Update() {
    if (!m_Visible || m_Buttons.empty()) return -1;

    const bool prev = Util::Input::IsKeyUp(Util::Keycode::UP)   || Util::Input::IsKeyUp(Util::Keycode::LEFT)  ||
                      Util::Input::IsKeyUp(Util::Keycode::W)    || Util::Input::IsKeyUp(Util::Keycode::A);
    const bool next = Util::Input::IsKeyUp(Util::Keycode::DOWN) || Util::Input::IsKeyUp(Util::Keycode::RIGHT) ||
                      Util::Input::IsKeyUp(Util::Keycode::S)    || Util::Input::IsKeyUp(Util::Keycode::D);

    if (prev) { m_Selected = StepToEnabled(m_Selected, -1); UpdateCursor(); }
    else if (next) { m_Selected = StepToEnabled(m_Selected, +1); UpdateCursor(); }

    if (Util::Input::IsKeyUp(Util::Keycode::SPACE) && m_Enabled[m_Selected]) {
        return m_Selected;
    }
    return -1;
}

int UIButtonList::StepToEnabled(int from, int dir) const {
    const int n = static_cast<int>(m_Buttons.size());
    if (n == 0) return 0;
    int idx = from;
    for (int i = 0; i < n; i++) {
        idx = (idx + dir + n) % n;
        if (m_Enabled[idx]) return idx;
    }
    return (from < 0) ? 0 : from;  // 全部停用：維持原樣
}

void UIButtonList::UpdateCursor() {
    for (size_t i = 0; i < m_Buttons.size(); i++) {
        const bool sel = (static_cast<int>(i) == m_Selected) && m_Highlight;
        m_Buttons[i]->SetDrawable(sel ? m_SelectedImg : m_NormalImg);

        Util::Color color = Util::Color::FromName(Util::Colors::BLACK);
        if (!m_Enabled[i]) color = Util::Color::FromName(Util::Colors::GRAY);
        else if (sel)      color = Util::Color::FromName(Util::Colors::WHITE);
        m_Labels[i]->SetColor(color);
    }
}
