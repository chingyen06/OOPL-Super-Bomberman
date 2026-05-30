#ifndef GAMECONSTANTS_HPP
#define GAMECONSTANTS_HPP

// All gameplay-tunable values live here so we don't bury magic numbers
// inside class implementations. Organised by domain. Time-based values
// are in frames (assuming a 60 FPS fixed step) unless suffixed Seconds.

namespace Constants {

    // ---------------- Game / round ----------------
    namespace Game {
        inline constexpr int kFPS                  = 60;
        inline constexpr int kRoundDurationSeconds = 180;            // 3 minutes
        inline constexpr int kRoundDurationFrames  = kFPS * kRoundDurationSeconds;
    }

    // ---------------- Player ----------------
    namespace Player {
        // Visual / collision
        inline constexpr float kSpriteWidth        = 36.0f;
        inline constexpr float kSpriteHeight       = 48.0f;
        inline constexpr float kSpriteYOffset      = 15.0f;          // visual lift above grid centre
        inline constexpr float kCollisionRadius    = 9.0f;

        // Movement
        inline constexpr float kNormalSpeed        = 3.0f;
        inline constexpr float kBoostSpeed         = 5.0f;
        inline constexpr int   kSpeedBoostFrames   = 300;            // 5 seconds

        // Loadout (start + cap)
        inline constexpr int   kInitialMaxBombs    = 3;
        inline constexpr int   kInitialFirepower   = 2;
        inline constexpr int   kMaxBombsCap        = 10;
        inline constexpr int   kFirepowerCap       = 5;

        // Lives / knockdown (防守方多命；前幾條被打中只會「倒地暈」一下而非死亡)
        inline constexpr int   kDefenderLives           = 3;
        inline constexpr int   kAttackerLives           = 1;
        inline constexpr int   kStunFrames              = 90;        // 倒地暈眩時間 (1.5s)
        inline constexpr int   kStunInvincibleAfter     = 60;        // 起身後短暫無敵 (1s)
        inline constexpr float kKnockdownRotation        = 1.5708f;   // 倒地時 sprite 旋轉 (約 90°)
        inline constexpr int   kKnockdownFallFrames      = 12;        // 由站到倒下的過渡 frame (漸進旋轉)
        inline constexpr float kKnockdownDrop            = 16.0f;     // 倒下時 sprite 往下沉的像素 (貼近地面)

        // Death / respawn
        inline constexpr int   kInvincibleFramesOnRespawn = 180;     // 3 seconds
        inline constexpr int   kDeathCountdownFrames      = 30;
        inline constexpr int   kRespawnDelayFrames        = 90;

        // Bomb-placement helper
        inline constexpr float kIgnoreBombClearance = 40.0f;         // px; players ignore bombs within this radius

        // BouncePad ride
        inline constexpr int   kBounceFrames        = 30;            // frames per bounce travel
        inline constexpr int   kBounceBlockedFrames = 15;            // frames when bouncing in place
        inline constexpr float kBounceJumpHeight    = 64.0f;
    }

    // ---------------- Bomb / Explosion ----------------
    namespace Bomb {
        inline constexpr int   kFuseFrames         = 180;            // 3 seconds
        inline constexpr int   kExplosionFrames    = 30;
        inline constexpr float kCollisionRadius    = 15.0f;
        inline constexpr float kAlignSpeed         = 1.5f;
        inline constexpr float kCenterSpeed        = 3.0f;
        inline constexpr float kIgnoreClearance    = 40.0f;          // px; nearby players auto-ignore the bomb
    }

    // ---------------- Turret ----------------
    namespace Turret {
        inline constexpr int kInitialIdleFrames    = 60;             // wait 1 s before first shot
        inline constexpr int kReadyFrames          = 30;             // visible "ready" pose duration
        inline constexpr int kCooldownFrames       = 60 * 5;         // 5 seconds between shots
        inline constexpr int kDefaultBombFirepower = 2;
        inline constexpr int kFireRangeMin         = 2;              // tiles
        inline constexpr int kFireRangeMax         = 3;
    }

    // ---------------- Projectile (turret bullet) ----------------
    namespace Projectile {
        inline constexpr float kSpriteScale        = 0.6f;
        inline constexpr float kSpeed              = 6.0f;           // px/frame
        inline constexpr float kHitRadius          = 18.0f;          // px; 飛行中擊中玩家的判定半徑
    }

    // ---------------- Bot (AI 控制的玩家) ----------------
    namespace Bot {
        // 兩次決策之間的最少 frame 數。值愈大反應愈慢、感覺愈不機械。
        // 不同 bot 用 playerID 做相位偏移避免每 K 幀同時 stutter。
        inline constexpr int kReactionFrames = 6;
    }

    // ---------------- Spirit (patrol enemy) ----------------
    namespace Spirit {
        inline constexpr int   kAlertRadius        = 5;              // tiles
        inline constexpr int   kPatrolInterval     = 60;             // frames between patrol steps
        inline constexpr int   kChaseInterval      = 30;             // frames between chase steps
        inline constexpr int   kPatrolRange        = 3;              // tiles around spawn
        inline constexpr float kMoveSpeed          = 1.0f;
        inline constexpr float kHoverAmplitude     = 5.0f;
        inline constexpr float kHoverSpeed         = 0.1f;
        inline constexpr float kVisualYOffset      = 10.0f;
    }

    // ---------------- Conveyor ----------------
    namespace Conveyor {
        inline constexpr float kPushSpeed          = 1.5f;
    }

    // ---------------- BouncePad ----------------
    namespace BouncePad {
        inline constexpr int kDefaultDistance      = 3;              // tiles to bounce
        inline constexpr int kCooldownFrames       = 60 * 5;         // 5 seconds
    }

    // ---------------- UI / HUD ----------------
    namespace UI {
        // 最多同時顯示鑰匙提示的玩家數 (= 整局最大玩家數上限)
        inline constexpr int kMaxKeyIndicators     = 15;
    }

} // namespace Constants

#endif
