#include "UIManager.hpp"
#include "Util/Image.hpp"
#include <cstdio>
#include <string>

UIImage::UIImage(const std::string& imagePath, int x, int y) {
    auto image = std::make_shared<Util::Image>(imagePath);
    SetDrawable(image);
    SetZIndex(99);
    m_Transform.translation = { x, y };
}

void UIImage::SetPosition(float x, float y) {
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

    m_KeyIndicators.clear();
}

void UIManager::Update(int gameTimeTicks, const std::vector<std::shared_ptr<Player>>& players, Util::Renderer& root) {
    int totalSeconds = gameTimeTicks / 60;
    int minutes = totalSeconds / 60;
    int seconds = totalSeconds % 60;

    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%02d:%02d", minutes, seconds);
    m_TimerText->SetText(std::string(buffer));

    int activeKeysNeeded = 0;

    for (const auto& player : players) {
        if (player->HasKey() && !player->IsDead()) {
            if (activeKeysNeeded >= m_KeyIndicators.size()) {
                auto newIndicator = std::make_shared<UIImage>(RESOURCE_DIR"/Image/key.png", -1000, -1000);
                newIndicator->SetZIndex(105);
                root.AddChild(newIndicator);
                m_KeyIndicators.push_back(newIndicator);
            }

            auto pos = player->GetPixelPos();
            m_KeyIndicators[activeKeysNeeded]->SetPosition(pos.x, pos.y + 50.0f);

            activeKeysNeeded++;
        }
    }

    for (size_t i = activeKeysNeeded; i < m_KeyIndicators.size(); i++) {
        m_KeyIndicators[i]->SetPosition(-1000.0f, -1000.0f);
    }
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

    for (auto& indicator : m_KeyIndicators) {
        root.RemoveChild(indicator);
    }
    m_KeyIndicators.clear();
}