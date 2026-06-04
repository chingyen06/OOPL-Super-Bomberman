#include "KeyBindings.hpp"

#include <fstream>
#include <string>
#include <nlohmann/json.hpp>

#include "Util/Logger.hpp"

// 與 save.json 同樣放在 RESOURCE_DIR：Debug 為編譯期絕對路徑、Release 為 exe 旁的 Resources/。
static const char* kKeysPath = RESOURCE_DIR"/keybindings.json";

void KeyBindings::Load() {
    std::ifstream file(kKeysPath);
    if (!file) {
        LOG_INFO("keybindings.json not found; using default key bindings");
        return;
    }
    try {
        nlohmann::json j;
        file >> j;
        // p0 / p1：各 kActions 個鍵 (上/下/左/右/放炸彈/武器)，以整數 scancode 存放。
        for (int pl = 0; pl < 2; ++pl) {
            const std::string key = "p" + std::to_string(pl);
            if (!j.contains(key)) continue;
            const auto& arr = j[key];
            for (int a = 0; a < kActions && a < static_cast<int>(arr.size()); ++a) {
                Key(pl, a) = static_cast<Util::Keycode>(arr[a].get<int>());
            }
        }
        if (j.contains("pause")) pause = static_cast<Util::Keycode>(j["pause"].get<int>());
        LOG_INFO("Loaded key bindings");
    }
    catch (...) {
        LOG_WARN("keybindings.json parse failed; resetting to default key bindings");
        *this = KeyBindings{};  // 還原為預設鍵
    }
}

void KeyBindings::Save() const {
    nlohmann::json j;
    for (int pl = 0; pl < 2; ++pl) {
        nlohmann::json arr = nlohmann::json::array();
        for (int a = 0; a < kActions; ++a) arr.push_back(static_cast<int>(Key(pl, a)));
        j["p" + std::to_string(pl)] = arr;
    }
    j["pause"] = static_cast<int>(pause);

    std::ofstream file(kKeysPath);
    if (!file) {
        LOG_WARN("cannot write keybindings.json");
        return;
    }
    file << j.dump(2);
}
