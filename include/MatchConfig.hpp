#ifndef MATCHCONFIG_HPP
#define MATCHCONFIG_HPP

// 一場對戰的可調設定 (由「更換規則」「選擇隊伍」畫面修改，GameSession::LoadLevel 套用)。
// 封裝成獨立類別 (SRP)，以 getter/setter 存取，預設值對應原本寫死的行為。
class MatchConfig {
public:
    // 進攻方席位模式 (enum class，避免裸 enum 汙染命名空間)。Human 僅用於玩家 2 席位 (slot 0)。
    enum class SlotMode { Off, Computer, Human };

    static constexpr int kMaxAttackers = 8;  // 進攻方上限 (= 席位數；slot 0 = 玩家 2，1..7 = 電腦)

    MatchConfig() {
        for (int i = 0; i < kMaxAttackers; i++) m_Slots[i] = SlotMode::Computer;  // 預設全 AI
    }

    // -------- 規則 --------
    int  RoundSeconds() const { return m_RoundSeconds; }
    void SetRoundSeconds(int s) { m_RoundSeconds = s; }

    bool SpiritsEnabled() const { return m_Spirits; }
    void SetSpiritsEnabled(bool b) { m_Spirits = b; }

    bool TurretsEnabled() const { return m_Turrets; }
    void SetTurretsEnabled(bool b) { m_Turrets = b; }

    // -------- 隊伍 (玩家 1 永遠是唯一守方；此處只設定進攻方席位) --------
    SlotMode AttackerSlot(int i) const {
        return (i >= 0 && i < kMaxAttackers) ? m_Slots[i] : SlotMode::Off;
    }
    void SetAttackerSlot(int i, SlotMode m) {
        if (i >= 0 && i < kMaxAttackers) m_Slots[i] = m;
    }

    bool Player2IsHuman() const { return m_Slots[0] == SlotMode::Human; }

    int AttackerTotal() const {
        int n = 0;
        for (int i = 0; i < kMaxAttackers; i++) if (m_Slots[i] != SlotMode::Off) n++;
        return n;
    }

private:
    int  m_RoundSeconds = 180;   // 預設 3 分鐘 (= 原本的 kRoundDurationSeconds)
    bool m_Spirits      = true;
    bool m_Turrets      = true;

    SlotMode m_Slots[kMaxAttackers];  // 進攻方席位 (建構子初始化為全 AI)
};

#endif
