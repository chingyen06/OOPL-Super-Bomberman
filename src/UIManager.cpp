#include "UIManager.hpp"

UIImage::UIImage(const std::string& imagePath, int x, int y) {
    auto image = std::make_shared<Util::Image>(imagePath);
    SetDrawable(image);
    SetZIndex(99);

    float targetWidth = 200.0f;
    float targetHeight = 40.0f;

    m_Transform.scale = {
        targetWidth / image->GetSize().x,
        targetHeight / image->GetSize().y
    };

    m_Transform.translation = { x, y };
}

UIText::UIText(const std::string& text, int x, int y) {
    m_Text = std::make_shared<Util::Text>(RESOURCE_DIR"/Font/GenJyuuGothicX-Bold.ttf", 30, text, Util::Color::FromName(Util::Colors::BLACK));

    SetDrawable(m_Text);
    SetZIndex(100);

    m_Transform.translation = { x, y };
}

void UIText::SetText(const std::string& text) {
    m_Text->SetText(text);
}

void UIManager::Init(Util::Renderer& root) {
    m_TimerBackground = std::make_shared<UIImage>(RESOURCE_DIR"/Image/timer.png", 0, 320);
    root.AddChild(m_TimerBackground);

    m_TimerText = std::make_shared<UIText>("03:00", 10, 320);
    root.AddChild(m_TimerText);
}

void UIManager::Update(int gameTimeTicks, const std::vector<std::shared_ptr<Player>>& players) {
    int totalSeconds = gameTimeTicks / 60;
    int minutes = totalSeconds / 60;
    int seconds = totalSeconds % 60;

    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%02d:%02d", minutes, seconds);
    m_TimerText->SetText(std::string(buffer));

    // 預留給更新玩家數值與鑰匙提示
}

void UIManager::Clear(Util::Renderer& root) {
    if (m_TimerText) {
        root.RemoveChild(m_TimerText);
        m_TimerText.reset();
    }

    if (m_TimerBackground) {
        root.RemoveChild(m_TimerBackground);
        m_TimerBackground.reset();
    }
}