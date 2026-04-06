#include "UIManager.hpp"
#include "Util/Image.hpp"
#include "Util/Logger.hpp"
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

void UIManager::Init(Util::Renderer& root, int totalChests) {
    Clear(root);

    m_TimerBackground = std::make_shared<UIImage>(RESOURCE_DIR"/Image/timer.png", 0, 320);
    root.AddChild(m_TimerBackground);

    m_CrownImage = std::make_shared<UIImage>(RESOURCE_DIR"/Image/crown.png", -1000, -1000);
    root.AddChild(m_CrownImage);

    m_TimerText = std::make_shared<UIText>("03:00", 10, 320);
    root.AddChild(m_TimerText);

    for (int i = 0; i < 15; i++) {
        auto indicator = std::make_shared<UIImage>(RESOURCE_DIR"/Image/key.png", -1000, -1000);
        root.AddChild(indicator);
        m_KeyIndicators.push_back(indicator);
    }

    m_ChestPool.clear();
    float startX = -30.0f;
    float startY = 285.0f;

    for (int i = 0; i < totalChests; i++) {
        auto chest = std::make_shared<UIImage>(RESOURCE_DIR"/Image/chest_closed.png", startX + (i * 30), startY);
        root.AddChild(chest);
        m_ChestPool.push_back(chest);
    }
}

void UIManager::Update(int gameTimeTicks, const std::vector<std::shared_ptr<Player>>& players, const std::vector<bool>& chestStatus, Util::Renderer& root) {
    int totalSeconds = gameTimeTicks / 60;

    if (totalSeconds != m_LastSeconds) {
        int minutes = totalSeconds / 60;
        int seconds = totalSeconds % 60;

        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%02d:%02d", minutes, seconds);
        m_TimerText->SetText(std::string(buffer));

        m_LastSeconds = totalSeconds; // 更新快取
    }

	// 皇冠
    bool defenderFound = false;
    for (const auto& player : players) {
        if (player->GetTeam() == Team::DEFENDER && !player->IsDead()) {
            auto pos = player->GetPixelPos();
            m_CrownImage->SetPosition(pos.x, pos.y + 42.0f);
            defenderFound = true;
            break;
        }
    }
    if (!defenderFound) {
        m_CrownImage->SetPosition(-1000.0f, -1000.0f);
    }
    
    // 鑰匙
    int activeKeysNeeded = 0;
    for (const auto& player : players) {
        if (player->HasKey() && !player->IsDead()) {
            if (activeKeysNeeded < m_KeyIndicators.size()) {
                auto pos = player->GetPixelPos();
                m_KeyIndicators[activeKeysNeeded]->SetPosition(pos.x, pos.y + 42.0);
                activeKeysNeeded++;
            }
        }
    }
    for (size_t i = activeKeysNeeded; i < m_KeyIndicators.size(); i++) {
        m_KeyIndicators[i]->SetPosition(-1000.0f, -1000.0f);
    }

	// 寶箱
    if (chestStatus != m_LastChestStatus) {
        size_t loopSize = std::min(chestStatus.size(), m_ChestPool.size());

        for (size_t i = 0; i < loopSize; i++) {
            if (chestStatus[i]) {
                auto openedImg = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/chest_opened.png");
                m_ChestPool[i]->SetDrawable(openedImg);
            }
        }
        m_LastChestStatus = chestStatus; // 更新快取
    }
}

void UIManager::Clear(Util::Renderer& root) {
    if (m_TimerBackground) {
        root.RemoveChild(m_TimerBackground);
        m_TimerBackground.reset();
    }

    if (m_CrownImage) {
        root.RemoveChild(m_CrownImage);
        m_CrownImage.reset();
    }

    if (m_TimerText) {
        root.RemoveChild(m_TimerText);
        m_TimerText.reset();
    }

    for (auto& indicator : m_KeyIndicators) {
        root.RemoveChild(indicator);
    }
    m_KeyIndicators.clear();

    for (auto& chest : m_ChestPool) {
        root.RemoveChild(chest);
    }
    m_ChestPool.clear();
}