#include "Menu.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"

void Menu::AddOption(const std::string& text, std::function<void()> onSelect) {
    m_RawTexts.push_back(text);
    auto uiText = std::make_shared<UIText>(text, 0, 0);
    m_OptionTexts.push_back(uiText);
    m_Callbacks.push_back(onSelect);
}

void Menu::Show(Util::Renderer& root, float startX, float startY) {
    if (m_IsVisible) return;
    m_IsVisible = true;
    m_SelectedIndex = 0;

    // 由上往下排版
    for (size_t i = 0; i < m_OptionTexts.size(); i++) {
        m_OptionTexts[i]->SetPosition(startX, startY - (i * 60.0f));
        root.AddChild(m_OptionTexts[i]);
    }
    UpdateCursor();
}

void Menu::Hide(Util::Renderer& root) {
    if (!m_IsVisible) return;
    m_IsVisible = false;
    for (auto& text : m_OptionTexts) {
        root.RemoveChild(text);
    }
}

void Menu::Update() {
    if (!m_IsVisible || m_OptionTexts.empty()) return;

    bool changed = false;

    // 處理上下移動
    if (Util::Input::IsKeyUp(Util::Keycode::UP) || Util::Input::IsKeyUp(Util::Keycode::W)) {
        m_SelectedIndex--;
        if (m_SelectedIndex < 0) m_SelectedIndex = m_OptionTexts.size() - 1;
        changed = true;
    }
    else if (Util::Input::IsKeyUp(Util::Keycode::DOWN) || Util::Input::IsKeyUp(Util::Keycode::S)) {
        m_SelectedIndex++;
        if (m_SelectedIndex >= static_cast<int>(m_OptionTexts.size())) m_SelectedIndex = 0;
        changed = true;
    }

    if (changed) 
        UpdateCursor();

    if (Util::Input::IsKeyUp(Util::Keycode::SPACE) || Util::Input::IsKeyUp(Util::Keycode::KP_ENTER) || Util::Input::IsKeyUp(Util::Keycode::RETURN)) {
        if (m_Callbacks[m_SelectedIndex]) {
            m_Callbacks[m_SelectedIndex]();
        }
    }
}

void Menu::UpdateCursor() {
    for (size_t i = 0; i < m_OptionTexts.size(); i++) {
        if (static_cast<int>(i) == m_SelectedIndex) {
            m_OptionTexts[i]->SetText("> " + m_RawTexts[i] + " <");
        }
        else {
            m_OptionTexts[i]->SetText("  " + m_RawTexts[i] + "  ");
        }
    }
}