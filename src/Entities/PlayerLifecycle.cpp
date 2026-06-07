#include "Entities/PlayerLifecycle.hpp"

#include "GameConstants.hpp"

PlayerLifecycle::PlayerLifecycle(int maxLives) : m_MaxLives(maxLives), m_Lives(maxLives) {}

int PlayerLifecycle::InvincibleAfterStunFrames() const {
    return Constants::Player::kStunInvincibleAfter;
}

PlayerLifecycle::TickStatus PlayerLifecycle::Tick() {
    TickStatus s;

    // 真正死亡：死亡倒數 → (時間到 hide + 進入重生倒數) → 重生倒數結束 → 通知 Player Respawn
    if (m_IsDead) {
        if (m_DeathCountdown > 0) {
            m_DeathCountdown--;
            if (m_DeathCountdown == 0) {
                s.hideSpriteNow  = true;
                m_RespawnTimer   = Constants::Player::kRespawnDelayFrames;
                m_DeathCountdown = -1;
            }
        }
        else if (m_RespawnTimer > 0) {
            m_RespawnTimer--;
            if (m_RespawnTimer == 0) {
                s.respawnNow   = true;
                m_RespawnTimer = -1;
            }
        }
        s.skipMovement = true;
        return s;
    }

    // 倒地暈眩：呼叫端依 stunFramesElapsed 套用視覺 (旋轉 / 下沉 / 閃爍)；起身那一幀短暫無敵
    if (m_StunTimer > 0) {
        m_StunTimer--;
        s.stunFramesLeft    = m_StunTimer;
        s.stunFramesElapsed = Constants::Player::kStunFrames - m_StunTimer;
        s.skipMovement      = true;
        if (m_StunTimer == 0) {
            s.stunJustEnded = true;
            m_Invincible    = InvincibleAfterStunFrames();
        }
        return s;
    }

    // 一般幀：只推進無敵計時
    if (m_Invincible > 0) m_Invincible--;
    return s;
}

PlayerLifecycle::HitOutcome PlayerLifecycle::OnHit(bool bounceActive) {
    // 作弊無敵 / 重生無敵 / 暈眩中 / 彈跳飛行中 (可跨過炸彈與火焰) 皆免疫
    if (m_GodMode || m_Invincible > 0 || m_StunTimer > 0 || bounceActive) return HitOutcome::Immune;

    m_Lives--;
    if (m_Lives > 0) {
        EnterKnockdown();
        return HitOutcome::KnockedDown;
    }
    EnterDeathFlow();
    return HitOutcome::Killed;
}

PlayerLifecycle::HitOutcome PlayerLifecycle::OnDebugKill() {
    if (m_IsDead) return HitOutcome::Immune;
    // 無視所有免疫狀態，強制把命數打到 0 → 走真正的死亡流程
    m_GodMode    = false;
    m_Invincible = -1;
    m_StunTimer  = -1;
    m_Lives      = 1;  // 接著 OnHit() 會遞減為 0 並進入死亡流程
    return OnHit(/*bounceActive=*/false);
}

void PlayerLifecycle::OnRespawned() {
    m_IsDead      = false;
    m_Lives       = m_MaxLives;
    m_StunTimer   = -1;
    m_Invincible  = Constants::Player::kInvincibleFramesOnRespawn;
}

void PlayerLifecycle::EnterDeathFlow() {
    m_IsDead         = true;
    m_DeathCountdown = Constants::Player::kDeathCountdownFrames;
}

void PlayerLifecycle::EnterKnockdown() {
    m_StunTimer = Constants::Player::kStunFrames;
}
