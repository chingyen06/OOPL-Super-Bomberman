#ifndef GAMECONSTANTS_HPP
#define GAMECONSTANTS_HPP

// All gameplay-tunable values live here so we don't bury magic numbers
// inside class implementations. Organised by domain via nested classes.
// Time-based values are in frames (assuming a 60 FPS fixed step) unless
// suffixed Seconds. Usage is unchanged, e.g. Constants::Game::kFPS.
//
// 傳統 OOP：以 class + 巢狀 class + static constexpr 取代 namespace。

class Constants {
public:
    // ---------------- Game / round ----------------
    class Game {
    public:
        static constexpr int kFPS                  = 60;
        static constexpr int kRoundDurationSeconds = 180;            // 3 minutes
        static constexpr int kRoundDurationFrames  = kFPS * kRoundDurationSeconds;
    };

    // ---------------- Player ----------------
    class Player {
    public:
        // Visual / collision
        static constexpr float kSpriteWidth        = 36.0f;
        static constexpr float kSpriteHeight       = 48.0f;
        static constexpr float kSpriteYOffset      = 15.0f;          // visual lift above grid centre
        static constexpr float kCollisionRadius    = 9.0f;

        // Movement
        static constexpr float kNormalSpeed        = 3.0f;
        static constexpr float kBoostSpeed         = 5.0f;
        static constexpr int   kSpeedBoostFrames   = 300;            // 5 seconds

        // Loadout (start + cap)
        static constexpr int   kInitialMaxBombs    = 3;
        static constexpr int   kInitialFirepower   = 2;
        static constexpr int   kMaxBombsCap        = 10;
        static constexpr int   kFirepowerCap       = 5;

        // Lives / knockdown (防守方多命；前幾條被打中只會「倒地暈」一下而非死亡)
        static constexpr int   kDefenderLives           = 3;
        static constexpr int   kAttackerLives           = 1;
        static constexpr int   kStunFrames              = 90;        // 倒地暈眩時間 (1.5s)
        static constexpr int   kStunInvincibleAfter     = 60;        // 起身後短暫無敵 (1s)
        static constexpr float kKnockdownRotation        = 1.5708f;   // 倒地時 sprite 旋轉 (約 90°)
        static constexpr int   kKnockdownFallFrames      = 12;        // 由站到倒下的過渡 frame (漸進旋轉)
        static constexpr float kKnockdownDrop            = 16.0f;     // 倒下時 sprite 往下沉的像素 (貼近地面)

        // Death / respawn
        static constexpr int   kInvincibleFramesOnRespawn = 180;     // 3 seconds
        static constexpr int   kDeathCountdownFrames      = 30;
        static constexpr int   kRespawnDelayFrames        = 90;

        // Bomb-placement helper
        static constexpr float kIgnoreBombClearance = 40.0f;         // px; players ignore bombs within this radius

        // BouncePad ride
        static constexpr int   kBounceFrames        = 30;            // frames per bounce travel
        static constexpr int   kBounceBlockedFrames = 15;            // frames when bouncing in place
        static constexpr float kBounceJumpHeight    = 64.0f;
    };

    // ---------------- Bomb / Explosion ----------------
    class Bomb {
    public:
        static constexpr int   kFuseFrames         = 180;            // 3 seconds
        static constexpr int   kExplosionFrames    = 30;
        static constexpr float kCollisionRadius    = 15.0f;
        static constexpr float kAlignSpeed         = 1.5f;
        static constexpr float kCenterSpeed        = 3.0f;
        static constexpr float kIgnoreClearance    = 40.0f;          // px; nearby players auto-ignore the bomb
    };

    // ---------------- Turret ----------------
    class Turret {
    public:
        static constexpr int kInitialIdleFrames    = 60;             // wait 1 s before first shot
        static constexpr int kReadyFrames          = 30;             // visible "ready" pose duration
        static constexpr int kCooldownFrames       = 60 * 5;         // 5 seconds between shots
        static constexpr int kDefaultBombFirepower = 2;
        static constexpr int kFireRangeMin         = 2;              // tiles
        static constexpr int kFireRangeMax         = 3;
    };

    // ---------------- Projectile (turret bullet) ----------------
    class Projectile {
    public:
        static constexpr float kSpriteScale        = 0.6f;
        static constexpr float kSpeed              = 6.0f;           // px/frame
        static constexpr float kHitRadius          = 18.0f;          // px; 飛行中擊中玩家的判定半徑
    };

    // ---------------- Bot (AI 控制的玩家) ----------------
    class Bot {
    public:
        // 兩次決策之間的最少 frame 數。值愈大反應愈慢、感覺愈不機械。
        // 不同 bot 用 playerID 做相位偏移避免每 K 幀同時 stutter。
        static constexpr int kReactionFrames = 6;
    };

    // ---------------- Spirit (patrol enemy) ----------------
    class Spirit {
    public:
        static constexpr int   kAlertRadius        = 5;              // tiles
        static constexpr int   kPatrolInterval     = 60;             // frames between patrol steps
        static constexpr int   kChaseInterval      = 30;             // frames between chase steps
        static constexpr int   kPatrolRange        = 3;              // tiles around spawn
        static constexpr float kMoveSpeed          = 1.0f;
        static constexpr float kHoverAmplitude     = 5.0f;
        static constexpr float kHoverSpeed         = 0.1f;
        static constexpr float kVisualYOffset      = 10.0f;
    };

    // ---------------- Conveyor ----------------
    class Conveyor {
    public:
        static constexpr float kPushSpeed          = 1.5f;
    };

    // ---------------- BouncePad ----------------
    class BouncePad {
    public:
        static constexpr int kDefaultDistance      = 3;              // tiles to bounce
        static constexpr int kCooldownFrames       = 60 * 5;         // 5 seconds
    };

    // ---------------- UI / HUD ----------------
    class UI {
    public:
        // 最多同時顯示鑰匙提示的玩家數 (= 整局最大玩家數上限)
        static constexpr int kMaxKeyIndicators     = 15;
    };
};

#endif
