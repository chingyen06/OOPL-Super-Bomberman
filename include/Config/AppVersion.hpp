#ifndef APP_VERSION_HPP
#define APP_VERSION_HPP

#include <string>

// 應用程式版本號的單一存取點。值來自 config.json (與 exe 的版本資訊 / 打包腳本同源)，
// 第一次呼叫時讀檔並快取，之後皆返回同一份字串。
//
// 為什麼放成獨立類別：版本號同時要出現在 (1) Windows exe 版本資訊 (gen_rc.ps1) (2) 打包
// 檔名 (package.ps1) (3) 標題畫面文字。三者都由 config.json 取得 → 升版只改一個地方。
class AppVersion {
public:
    // 取得 "1.1" / "1.2.3" 格式的版本字串。讀檔失敗會回 "0.0" 並記 warning。
    static const std::string& String();

private:
    AppVersion() = delete;
};

#endif
