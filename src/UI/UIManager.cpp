#include "UI/UIManager.hpp"
#include "GameConstants.hpp"
#include "Util/Image.hpp"
#include "Util/Logger.hpp"
#include <cstdio>
#include <string>
#include <algorithm>

void UIManager::Init(Util::Renderer& root, int totalChests) {
    Clear(root);

    // 預載一次「寶箱開啟」紋理，之後寶箱開啟時直接共用，不再每次讀檔
    if (!m_ChestOpenedImage) {
        m_ChestOpenedImage = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/chest_opened.png");
    }

    m_TimerBackground = std::make_shared<UIImage>(RESOURCE_DIR"/Image/timer.png", 0, 320);
    root.AddChild(m_TimerBackground);

    m_CrownImage = std::make_shared<UIImage>(RESOURCE_DIR"/Image/crown.png", -1000, -1000);
    root.AddChild(m_CrownImage);

    m_TimerText = std::make_shared<UIText>("03:00", 10, 320);
    root.AddChild(m_TimerText);

    for (int i = 0; i < Constants::UI::kMaxKeyIndicators; i++) {
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
    int totalSeconds = gameTimeTicks / Constants::Game::kFPS;

    if (totalSeconds != m_LastSeconds) {
        int minutes = totalSeconds / 60;
        int seconds = totalSeconds % 60;

        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%02d:%02d", minutes, seconds);
        m_TimerText->SetText(std::string(buffer));

        m_LastSeconds = totalSeconds; // Update cache
    }

    // Crown indicator on the surviving defender
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
    
    // Key indicators floating above players holding a key
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

    // Chest icons in HUD
    if (chestStatus != m_LastChestStatus) {
        size_t loopSize = std::min(chestStatus.size(), m_ChestPool.size());

        for (size_t i = 0; i < loopSize; i++) {
            bool wasOpened = (m_LastChestStatus.size() > i) ? m_LastChestStatus[i] : false;
            bool isOpened = chestStatus[i];

            if (isOpened && !wasOpened) {
                m_ChestPool[i]->SetDrawable(m_ChestOpenedImage);
            }
        }
        m_LastChestStatus = chestStatus; // Update cache
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