#include "UI/ButtonRow.hpp"

void ButtonRow::SetButtonImages(const std::shared_ptr<Util::Image>& normal,
                                const std::shared_ptr<Util::Image>& selected) {
    m_BtnNormal   = normal;
    m_BtnSelected = selected;
}

int ButtonRow::AddButton(const std::string& text, float buttonZ, float labelZ) {
    auto button = std::make_shared<UIImage>(0.0f, 0.0f, buttonZ);
    button->SetDrawable(m_BtnNormal);
    auto label = std::make_shared<UIText>(text, 0.0f, 0.0f, labelZ, LabelColorNormal());

    m_Texts.push_back(text);
    m_Buttons.push_back(button);
    m_Labels.push_back(label);
    m_Enabled.push_back(true);
    return static_cast<int>(m_Buttons.size()) - 1;
}

void ButtonRow::SetLabel(int index, const std::string& text) {
    if (index < 0 || index >= static_cast<int>(m_Labels.size())) return;
    m_Texts[index] = text;
    m_Labels[index]->SetText(text);
}

void ButtonRow::SetButtonEnabled(int index, bool enabled) {
    if (index < 0 || index >= static_cast<int>(m_Enabled.size())) return;
    m_Enabled[index] = enabled;
}

void ButtonRow::AttachButtons(Util::Renderer& root) {
    for (auto& b : m_Buttons) root.AddChild(b);
    for (auto& l : m_Labels)  root.AddChild(l);
}

void ButtonRow::DetachButtons(Util::Renderer& root) {
    for (auto& b : m_Buttons) root.RemoveChild(b);
    for (auto& l : m_Labels)  root.RemoveChild(l);
}

void ButtonRow::UpdateCursor() {
    for (size_t i = 0; i < m_Buttons.size(); i++) {
        const bool sel = (static_cast<int>(i) == m_Selected) && m_Highlight;
        m_Buttons[i]->SetDrawable(sel ? m_BtnSelected : m_BtnNormal);

        const Util::Color color = !m_Enabled[i] ? LabelColorDisabled()
                                : sel           ? LabelColorSelected()
                                                : LabelColorNormal();
        m_Labels[i]->SetColor(color);
    }
}
