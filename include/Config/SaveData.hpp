#ifndef SAVEDATA_HPP
#define SAVEDATA_HPP

// 玩家存檔：目前只保存金幣總額，關掉遊戲重開仍會保留 (對齊原版的金幣累計)。
// 以 nlohmann/json 寫到 RESOURCE_DIR"/save.json"。RESOURCE_DIR 是編譯期的絕對路徑，
// 不受執行時工作目錄影響，因此讀寫位置永遠一致。
class SaveData {
public:
    void Load();                 // 啟動時讀檔；檔案不存在 → 金幣為 0
    int  Coins() const { return m_Coins; }
    void AddCoins(int amount);   // 加錢 (不會變負) 並立即存檔
    void SetCoins(int amount);   // 直接設定金幣 (debug 主控台用) 並存檔

private:
    void Save() const;
    int m_Coins = 0;
};

#endif
