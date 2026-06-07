#ifndef PLAYER_LIFECYCLE_HPP
#define PLAYER_LIFECYCLE_HPP

// 玩家生命/死亡/暈眩/無敵計時的狀態機 — 從 Player 拆出 (SRP)。
// 對外只負責「狀態查詢」與「驅動每幀的時鐘推進」，視覺呈現 (旋轉/閃爍) 仍由 Player
// 依 TickStatus 自行套用 — 避免 Lifecycle 反向依賴 PTSD Util::GameObject。
class PlayerLifecycle {
public:
    explicit PlayerLifecycle(int maxLives);

    // 公開狀態查詢 (Player 用同名 getter 轉發給 Lifecycle，外部 API 不變)
    bool IsDead() const     { return m_IsDead; }
    bool IsStunned() const  { return m_StunTimer > 0; }
    int  Lives() const      { return m_Lives; }
    bool IsGodMode() const  { return m_GodMode; }
    void SetGodMode(bool g) { m_GodMode = g; }

    // 一幀的推進結果，由 Player::Update 取得後決定要不要跑後續移動 / 動畫。
    class TickStatus {
    public:
        bool skipMovement      = false;  // true → Player 本幀直接 return (死亡流程或暈眩中)
        bool hideSpriteNow     = false;  // 進入「等待重生」那一幀 → Player 隱藏 sprite
        bool respawnNow        = false;  // 重生倒數結束這一幀 → Player 呼叫 Respawn()
        // 暈眩視覺資訊 (Player 自行套用 rotation / translation / visibility)
        int  stunFramesLeft    = -1;     // -1 = 未暈眩
        int  stunFramesElapsed = 0;      // 暈眩進入後經過幀數 (用於漸進倒下)
        bool stunJustEnded     = false;  // 起身那一幀 → Player 還原 rotation 並給無敵
    };
    TickStatus Tick();

    // 受擊。回傳本次結果：免疫 / 倒地暈眩 (還有命) / 真正死亡 (進入死亡倒數)。
    enum class HitOutcome { Immune, KnockedDown, Killed };
    HitOutcome OnHit(bool bounceActive);

    // Debug：無視免疫直接致死，走真正的死亡流程 (供主控台用)。
    HitOutcome OnDebugKill();

    // Respawn 結束後由 Player 呼叫，重置命數與狀態旗標。
    void OnRespawned();

    // 起身後設定的無敵帧數常數 (Player 呼叫，避免 Player 重複寫常數)
    int InvincibleAfterStunFrames() const;

private:
    int  m_MaxLives;
    int  m_Lives;
    bool m_IsDead         = false;
    bool m_GodMode        = false;
    int  m_Invincible     = -1;
    int  m_StunTimer      = -1;
    int  m_DeathCountdown = -1;
    int  m_RespawnTimer   = -1;

    void EnterDeathFlow();  // 真正死亡 → 設死亡倒數
    void EnterKnockdown();  // 倒地暈眩 → 設暈眩計時
};

#endif
