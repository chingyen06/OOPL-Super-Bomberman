#include "Config/AppVersion.hpp"

#include <fstream>
#include <nlohmann/json.hpp>

#include "Util/Logger.hpp"

const std::string& AppVersion::String() {
    // 第一次讀取後快取於 static — 之後重複呼叫只是回 reference。config.json 與 exe 同
    // 目錄 (main.cpp 啟動時已 chdir 至 exe 所在位置)，使用相對路徑即可，Debug 與 Release 一致。
    static std::string cached = []() {
        std::ifstream file("config.json");
        if (!file) {
            LOG_WARN("AppVersion: config.json not found; falling back to 0.0");
            return std::string{ "0.0" };
        }
        try {
            nlohmann::json j;
            file >> j;
            if (j.contains("version")) return j["version"].get<std::string>();
        } catch (...) {
            LOG_WARN("AppVersion: config.json parse failed; falling back to 0.0");
        }
        return std::string{ "0.0" };
    }();
    return cached;
}
