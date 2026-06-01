#include "SaveData.hpp"

#include <fstream>
#include <nlohmann/json.hpp>

#include "Util/Logger.hpp"

static const char* kSavePath = RESOURCE_DIR"/save.json";

void SaveData::Load() {
    std::ifstream file(kSavePath);
    if (!file) {
        LOG_INFO("save.json not found; starting with 0 coins");
        return;
    }
    try {
        nlohmann::json j;
        file >> j;
        if (j.contains("coins")) m_Coins = j["coins"].get<int>();
        if (m_Coins < 0) m_Coins = 0;
        // 背景音樂音量：相容舊存檔的 "musicVolume"
        if (j.contains("musicVolume")) m_BgmVolume = Clamp100(j["musicVolume"].get<int>());
        if (j.contains("bgmVolume"))   m_BgmVolume = Clamp100(j["bgmVolume"].get<int>());
        LOG_INFO("Loaded save: coins=" + std::to_string(m_Coins) +
                 " bgm=" + std::to_string(m_BgmVolume));
    }
    catch (...) {
        LOG_WARN("save.json parse failed; resetting coins to 0");
        m_Coins = 0;
    }
}

void SaveData::Save() const {
    nlohmann::json j;
    j["coins"] = m_Coins;
    j["bgmVolume"] = m_BgmVolume;
    std::ofstream file(kSavePath);
    if (!file) {
        LOG_WARN("cannot write save.json");
        return;
    }
    file << j.dump(2);
}

static constexpr int kMaxCoins = 99'999;  // 上限：5 位數，避免右上角數字蓋到金幣圖示

void SaveData::AddCoins(int amount) {
    if (amount < 0 && -amount > m_Coins) amount = -m_Coins;  // 不為負
    m_Coins += amount;
    if (m_Coins > kMaxCoins) m_Coins = kMaxCoins;
    Save();
}

void SaveData::SetCoins(int amount) {
    m_Coins = amount < 0 ? 0 : (amount > kMaxCoins ? kMaxCoins : amount);
    Save();
}

void SaveData::SetBgmVolume(int percent) { m_BgmVolume = Clamp100(percent); Save(); }
