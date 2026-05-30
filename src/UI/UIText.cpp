#include "UI/UIText.hpp"

UIText::UIText(const std::string& text, float x, float y, float z, const Util::Color& color) {
    m_Text = std::make_shared<Util::Text>(RESOURCE_DIR"/Font/GenJyuuGothicX-Bold.ttf", 30, text, color);

    SetDrawable(m_Text);
    SetZIndex(z);
    m_Transform.translation = { x, y };
}

void UIText::SetText(const std::string& text) {
    m_Text->SetText(text);
}

void UIText::SetPosition(float x, float y) {
    m_Transform.translation = { x, y };
}

void UIText::SetColor(const Util::Color& color) {
    m_Text->SetColor(color);
}
