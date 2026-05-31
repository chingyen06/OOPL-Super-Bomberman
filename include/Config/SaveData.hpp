#ifndef SAVEDATA_HPP
#define SAVEDATA_HPP

// 玩家存檔：保存金幣總額與音樂音量，關掉遊戲重開仍會保留。
// 以 nlohmann/json 寫到 RESOURCE_DIR"/save.json"。RESOURCE_DIR 是編譯期的絕對路徑，
// 不受執行時工作目錄影響，因此讀寫位置永遠一致。
class SaveData {
public:
    void Load();                 // 啟動時讀檔；檔案不存在 → 金幣為 0、音量為預設
    int  Coins() const { return m_Coins; }
    void AddCoins(int amount);   // 加錢 (不會變負) 並立即存檔
    void SetCoins(int amount);   // 直接設定金幣 (debug 主控台用) 並存檔

    // 背景音樂音量 (0..100 百分比)，與金幣一樣持久化到 save.json。
    int  BgmVolume() const { return m_BgmVolume; }
    void SetBgmVolume(int percent);  // clamp 0..100 並立即存檔

private:
    void Save() const;
    static int Clamp100(int v) { return v < 0 ? 0 : (v > 100 ? 100 : v); }

    int m_Coins = 0;
    int m_BgmVolume = 60;  // 預設 60%
};

#endif
