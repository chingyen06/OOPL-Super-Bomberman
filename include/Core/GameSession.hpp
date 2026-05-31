#ifndef GAMESESSION_HPP
#define GAMESESSION_HPP

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <chrono>

#include "UI/UIImage.hpp"
#include "Weapons/IDefenderWeapon.hpp"
#include "Weapons/IWeaponEffects.hpp"
#include "AIManager.hpp"
#include "BombManager.hpp"
#include "CheatManager.hpp"
#include "DebugOverlay.hpp"
#include "MatchConfig.hpp"
#include "InteractableManager.hpp"
#include "LevelManager.hpp"
#include "Player.hpp"
#include "Spirit.hpp"
#include "Turret/TurretManager.hpp"
#include "UI/UIManager.hpp"
#include "Util/Renderer.hpp"

class SaveData;
class KeyBindings;

// 一場對戰結束時的結算資料 (供結算畫面顯示 +金幣 與表現明細)。
class MatchResult {
public:
    bool defenderWin    = false;  // true = 防守方(玩家1)獲勝
    int  totalChests    = 0;
    int  chestsOpened   = 0;      // 被進攻方開啟的寶箱數
    int  chestsDefended = 0;      // 守住 (未被開啟) 的寶箱數
    int  elapsedSeconds = 0;      // 本回合經過秒數
    int  defenderKills  = 0;      // 防守方用武器擊倒進攻方數
    int  coinsEarned    = 0;      // 本回合獲得金幣
};

// 一次性的對戰會話：持有所有 gameplay-side 的 manager 與 entity。
// App 不再直接觸碰這些成員 (移除 friend)；State 透過 App::Session() 取得 reference。
// 同時實作 IWeaponEffects：武器透過它生成短暫特效 (由本類別管理生命週期)。
class GameSession : public IWeaponEffects {
public:
    explicit GameSession(Util::Renderer& root);

    // IWeaponEffects：武器特效 (短暫顯示後自動移除)
    void AddEffect(float px, float py, const std::string& sprite, int frames) override;

    void LoadLevel(int levelIndex);  // 載入關卡 + spawn 玩家、Spirit、砲台
    void Update();                   // 推進一個 tick (manager + entity)
    void Clear();                    // 把所有實體從場景移除 (GameEnd 用)

    // 勝負查詢
    bool IsAttackerWin() const;
    bool IsTimeUp() const { return m_GameTime == 0; }

    // 依目前局勢產生結算資料 (含金幣獎勵公式)。在 Clear() 之前呼叫。
    MatchResult BuildResult(bool defenderWin) const;

    // debug 主控台需要讀寫金幣；由 App 在啟動時注入存檔指標 (非擁有)。
    void SetProfile(SaveData* profile) { m_Profile = profile; }
    void SetKeyBindings(KeyBindings* keys) { m_Keys = keys; }  // 玩家按鍵設定 (非擁有)

    // 暫停選單用：作弊模式開關 (idx 0 = 玩家1，1 = 玩家2) + 目前關卡 (供「再次開始」重載)
    void ToggleCheat(int idx)          { m_CheatManager.Toggle(idx); }
    bool IsCheatEnabled(int idx) const { return m_CheatManager.IsEnabled(idx); }
    bool HasHumanPlayer2() const       { return m_HumanPlayer2 != nullptr; }
    int  GetCurrentLevel() const       { return m_CurrentLevelIndex; }

    bool IsDebugOpen() const { return m_DebugOverlay.IsEnabled(); }  // 滑鼠顯示判斷用

    // 「更換規則」「選擇隊伍」畫面透過此設定影響下一場對戰
    MatchConfig& Config() { return m_Config; }

private:
    void RenderDebugConsole();    // F3 開啟的 ImGui debug 主控台 (在 Update 內提交)

    // 找一格可放掉落鑰匙的位置：避免和炸彈同格 (會把人砸倒)、避免和既有鑰匙/物件重疊。
    std::pair<int, int> FindKeyDropTile(int sx, int sy) const;

    Util::Renderer& m_Root;

    LevelManager        m_LevelManager;
    BombManager         m_BombManager;
    InteractableManager m_InteractableManager;
    AIManager           m_AIManager;
    TurretManager       m_TurretManager;
    UIManager           m_UIManager;
    CheatManager        m_CheatManager;
    DebugOverlay        m_DebugOverlay;

    std::vector<std::shared_ptr<Player>> m_Players;
    std::vector<std::shared_ptr<Spirit>> m_Spirits;

    Player* m_HumanPlayer  = nullptr;  // 玩家1 (人類防守方)；作弊/HUD 對象，指向 m_Players 內物件
    Player* m_HumanPlayer2 = nullptr;  // 玩家2 (若為人類攻擊方，否則 nullptr)

    MatchConfig m_Config;         // 規則 / 陣營設定
    int m_CurrentLevelIndex = 1;  // 最近載入的關卡 (「再次開始」用)
    int m_GameTime = -1;

    // debug overlay 用的 FPS 量測 (以實際更新間隔平滑估算)
    std::chrono::steady_clock::time_point m_LastFrameTime{};
    float m_Fps = 60.0f;
    bool  m_HasFrameTime = false;

    bool         m_ShowDanger = true;    // debug 主控台勾選：是否疊危險地圖紅塊
    SaveData*    m_Profile    = nullptr; // 金幣存檔 (非擁有；App 注入) — 主控台讀寫用
    KeyBindings* m_Keys       = nullptr; // 玩家按鍵設定 (非擁有；App 注入)

    // ---- 防守方武器 + 充能槽 ----
    std::unique_ptr<IDefenderWeapon> m_Weapon;     // 本場武器 (工廠依 MatchConfig 建立)
    float m_Charge        = 0.0f;                  // 充能 0..1
    int   m_DefenderKills = 0;                     // 武器擊倒進攻方累計 (計入金幣)
    std::shared_ptr<UIImage> m_ChargeBg;           // 充能條背景 (浮在防守方頭上)
    std::shared_ptr<UIImage> m_ChargeFill;         // 充能條填充
    std::vector<std::pair<std::shared_ptr<UIImage>, int>> m_WeaponFx;  // 武器特效 + 剩餘 frame

    void UpdateDefenderWeapon();  // 充能 / 發動 / 特效 / 充能條，每幀呼叫
};

#endif
