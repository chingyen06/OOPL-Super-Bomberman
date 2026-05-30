#include "UI/UIManager.hpp"
#include "GameConstants.hpp"
#include "Util/Color.hpp"
#include "Util/Image.hpp"
#include "Util/Logger.hpp"
#include <cstdio>
#include <string>
#include <algorithm>

// 頂部 HUD 版面 (畫面中心為原點、+y 朝上)。z 用 90~95：在玩家/地圖之上、
// 但低於暫停變暗層 (97)，所以暫停時整個頂部 HUD 會一起變暗。
// 仿原版：計時器膠囊置中於頂部，目標寶箱圖示排在膠囊「正上方」一小排。
static constexpr float kTimerY       = 318.0f;  // 計時器膠囊中心 y
static constexpr float kChestRowY    = 349.0f;  // 目標寶箱列 y (膠囊上方)
static constexpr float kChestSize    = 18.0f;   // 寶箱顯示邊長 (原圖 16)
static constexpr float kChestSpacing = 24.0f;   // 寶箱間距

void UIManager::Init(Util::Renderer& root, int totalChests) {
    Clear(root);

    // 預載一次「寶箱開啟」紋理，之後寶箱開啟時直接共用，不再每次讀檔
    if (!m_ChestOpenedImage) {
        m_ChestOpenedImage = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/chest_opened.png");
    }

    // 頂部計時器膠囊 (置中) + 計時數字 (置中於膠囊內)
    m_TimerBackground = std::make_shared<UIImage>(RESOURCE_DIR"/Image/timer.png", 0.0f, kTimerY, 90.0f);
    root.AddChild(m_TimerBackground);

    m_CrownImage = std::make_shared<UIImage>(RESOURCE_DIR"/Image/crown.png", -1000, -1000);
    root.AddChild(m_CrownImage);

    // 文字貼圖右側留白 → +5 才在膠囊內水平置中 (與選單同樣的補正)
    m_TimerText = std::make_shared<UIText>("3:00", 5.0f, kTimerY, 95.0f);
    root.AddChild(m_TimerText);

    // 作弊模式提示：預設隱藏，啟用時才顯示於螢幕左上
    m_CheatText = std::make_shared<UIText>("作弊模式", -480, kTimerY,
                                           95.0f, Util::Color::FromName(Util::Colors::RED));
    m_CheatText->SetVisible(false);
    root.AddChild(m_CheatText);
    m_LastCheatEnabled = false;

    for (int i = 0; i < Constants::UI::kMaxKeyIndicators; i++) {
        auto indicator = std::make_shared<UIImage>(RESOURCE_DIR"/Image/key.png", -1000, -1000);
        root.AddChild(indicator);
        m_KeyIndicators.push_back(indicator);
    }

    // 暈眩星星指示器 (數量與玩家上限相同；浮在被擊暈玩家頭上)
    for (int i = 0; i < Constants::UI::kMaxKeyIndicators; i++) {
        auto stun = std::make_shared<UIImage>(RESOURCE_DIR"/Image/stun_stars.png", -1000.0f, -1000.0f, 30.0f);
        root.AddChild(stun);
        m_StunIndicators.push_back(stun);
    }
    m_StunSpinTick = 0;

    // 目標列：寶箱圖示排成一橫排、置中於計時器正下方 (像原版頂部的目標圖示)
    m_ChestPool.clear();
    const float scale   = kChestSize / 16.0f;  // chest 原圖 16x16
    const float startX  = -(totalChests - 1) * kChestSpacing / 2.0f;

    for (int i = 0; i < totalChests; i++) {
        auto chest = std::make_shared<UIImage>(RESOURCE_DIR"/Image/chest_closed.png",
                                               startX + i * kChestSpacing, kChestRowY, 95.0f);
        chest->SetScale(scale, scale);
        root.AddChild(chest);
        m_ChestPool.push_back(chest);
    }
}

void UIManager::Update(int gameTimeTicks, const std::vector<std::shared_ptr<Player>>& players, const std::vector<bool>& chestStatus, Util::Renderer& root, bool cheatEnabled) {
    int totalSeconds = gameTimeTicks / Constants::Game::kFPS;

    if (cheatEnabled != m_LastCheatEnabled) {
        if (m_CheatText) m_CheatText->SetVisible(cheatEnabled);
        m_LastCheatEnabled = cheatEnabled;
    }

    if (totalSeconds != m_LastSeconds) {
        int minutes = totalSeconds / 60;
        int seconds = totalSeconds % 60;

        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%d:%02d", minutes, seconds);
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

    // 暈眩星星：浮在被擊暈 (倒地) 玩家頭上並緩慢旋轉，營造「暈」的感覺
    m_StunSpinTick++;
    const float spin = static_cast<float>(m_StunSpinTick) * 0.15f;
    size_t activeStuns = 0;
    for (const auto& player : players) {
        if (player->IsStunned() && !player->IsDead()) {
            if (activeStuns < m_StunIndicators.size()) {
                auto pos = player->GetPixelPos();
                m_StunIndicators[activeStuns]->SetPosition(pos.x, pos.y + 40.0f);
                m_StunIndicators[activeStuns]->SetRotation(spin);
                activeStuns++;
            }
        }
    }
    for (size_t i = activeStuns; i < m_StunIndicators.size(); i++) {
        m_StunIndicators[i]->SetPosition(-1000.0f, -1000.0f);
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

    if (m_CheatText) {
        root.RemoveChild(m_CheatText);
        m_CheatText.reset();
    }
    m_LastCheatEnabled = false;

    for (auto& indicator : m_KeyIndicators) {
        root.RemoveChild(indicator);
    }
    m_KeyIndicators.clear();

    for (auto& stun : m_StunIndicators) {
        root.RemoveChild(stun);
    }
    m_StunIndicators.clear();

    for (auto& chest : m_ChestPool) {
        root.RemoveChild(chest);
    }
    m_ChestPool.clear();
}