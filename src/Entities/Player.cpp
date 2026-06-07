#include "Player.hpp"
#include "GridCoord.hpp"
#include "Util/Logger.hpp"
#include <cmath>
#include <algorithm>

Player::Player(int startGridX, int startGridY, Team team, std::unique_ptr<InputController> controller, int id)
    : m_GridX(startGridX), m_GridY(startGridY),
      m_CurrentDir(Direction::DOWN),
      m_Lifecycle((team == Team::DEFENDER) ? Constants::Player::kDefenderLives
                                           : Constants::Player::kAttackerLives),
      m_Team(team), m_Controller(std::move(controller)),
      m_SpawnX(startGridX), m_SpawnY(startGridY),
      m_PlayerID(id) {

    auto initImg = m_Animator.InitialImage();
    SetDrawable(initImg); // 初始面向下方
    SetZIndex(20);

    // 放大角色至設定的 sprite 尺寸
    m_Transform.scale = {
        Constants::Player::kSpriteWidth  / initImg->GetSize().x,
        Constants::Player::kSpriteHeight / initImg->GetSize().y
    };

    // 計算初始像素座標
    m_Pos = GridCoord::ToPixel(m_GridX, m_GridY);

    m_Transform.translation = { m_Pos.x, m_Pos.y + Constants::Player::kSpriteYOffset };
}

void Player::ApplyStunVisuals(const PlayerLifecycle::TickStatus& s) {
    if (s.stunJustEnded) {
        m_Transform.rotation = 0.0f;
        m_Transform.translation = { std::round(m_Pos.x), std::round(m_Pos.y + Constants::Player::kSpriteYOffset) };
        SetVisible(true);
        return;
    }
    // 漸進倒下 + 下沉 + 閃爍
    const float t = std::min(1.0f, static_cast<float>(s.stunFramesElapsed) / Constants::Player::kKnockdownFallFrames);
    m_Transform.rotation = t * Constants::Player::kKnockdownRotation;
    m_Transform.translation = {
        std::round(m_Pos.x),
        std::round(m_Pos.y + Constants::Player::kSpriteYOffset - t * Constants::Player::kKnockdownDrop)
    };
    SetVisible((s.stunFramesLeft / 8) % 2 == 0);
}

void Player::Update(const IWorldContext& worldContext) {
    // 死亡 / 暈眩計時推進；視覺由本類別套用，狀態機本身不依賴 PTSD
    const auto lifecycleStatus = m_Lifecycle.Tick();
    if (lifecycleStatus.hideSpriteNow) SetVisible(false);
    if (lifecycleStatus.respawnNow)    Respawn();
    if (lifecycleStatus.stunFramesLeft >= 0) ApplyStunVisuals(lifecycleStatus);
    if (lifecycleStatus.skipMovement)  return;

    // 移動速度：加速道具 / AI 速度倍率
    float speed = Constants::Player::kNormalSpeed;
    if (m_SpeedBoostTimer > 0) {
        m_SpeedBoostTimer--;
        speed = Constants::Player::kBoostSpeed;
    }
    speed *= m_SpeedFactor;  // AI 進攻方 <1：讓人類防守方有機動優勢

    if (m_Bounce.IsActive()) {
        auto step = m_Bounce.Update();
        m_Pos = step.pos;
        if (step.finished) {
            m_GridX = GridCoord::ToGridX(m_Pos.x);
            m_GridY = GridCoord::ToGridY(m_Pos.y);
            m_Transform.translation = { m_Pos.x, m_Pos.y + Constants::Player::kSpriteYOffset };
        }
        else {
            m_Transform.translation = { m_Pos.x, m_Pos.y + Constants::Player::kSpriteYOffset + step.jumpHeight };
        }
        return;
    }

    float dx = 0.0f;
    float dy = 0.0f;
    float nextX = m_Pos.x;
    float nextY = m_Pos.y;

    // 取得當前位置的網格中心點座標
    float centerX = GridCoord::ToPixelX(m_GridX);
    float centerY = GridCoord::ToPixelY(m_GridY);

    bool isUpPressed = false;
    bool isDownPressed = false;
    bool isLeftPressed = false;
    bool isRightPressed = false;

    if (m_Controller) {
        isUpPressed = m_Controller->IsUpPressed();
        isDownPressed = m_Controller->IsDownPressed();
        isLeftPressed = m_Controller->IsLeftPressed();
        isRightPressed = m_Controller->IsRightPressed();
    }

    if (isUpPressed) {
        dy += speed;
        ChangeDirection(Direction::UP);
    }
    else if (isDownPressed) {
        dy -= speed;
        ChangeDirection(Direction::DOWN);
    }

    if (isLeftPressed) {
        dx -= speed;
        ChangeDirection(Direction::LEFT);
    }
    else if (isRightPressed) {
        dx += speed;
        ChangeDirection(Direction::RIGHT);
    }

    bool moveX = (dx != 0.0f);
    bool moveY = (dy != 0.0f);

    if (moveX && moveY) {  // 斜向移動速度要與單一方向移動的速度一致
        dx /= sqrt(2);
        dy /= sqrt(2);
    }

    nextX += dx;
    nextY += dy;

    glm::vec2 envForce = worldContext.GetForceAt(m_GridX, m_GridY);

    nextX += envForce.x;
    nextY += envForce.y;

    if (!moveY && envForce.y == 0.0f) {
        // 有橫向移動 (無論是主動走，還是被推)，把 Y 設到格子中央
        if (moveX || envForce.x != 0.0f) {
            if (m_Pos.y < centerY) nextY += std::min(speed, centerY - m_Pos.y);
            else if (m_Pos.y > centerY) nextY -= std::min(speed, m_Pos.y - centerY);
        }
    }

    if (!moveX && envForce.x == 0.0f) {
        // 有縱向移動 (無論是主動走，還是被推)，把 X 設到格子中央
        if (moveY || envForce.y != 0.0f) {
            if (m_Pos.x < centerX) nextX += std::min(speed, centerX - m_Pos.x);
            else if (m_Pos.x > centerX) nextX -= std::min(speed, m_Pos.x - centerX);
        }
    }

    bool isForced = (envForce.x != 0.0f || envForce.y != 0.0f);  // 被推動

    // 完全靜止 (沒按鍵、沒外力)：把人物收回「所在格」中心，避免停在格子邊角而半身
    // 留在隔壁格、被隔壁格的爆風波及 (AI 只走到某格角落就停尤其明顯)。僅 AI 啟用——
    // 人類玩家若被自動歸位，停下時會有「被硬拽到格中心」的差手感、又容易被預判利用。
    // std::min 夾住位移量，最多剛好到中心、不會越過，因此不會在中心左右抖動。
    constexpr float kCenterEps = 0.5f;
    bool centering = false;
    if (m_AutoCenterIdle && !moveX && !moveY && !isForced) {
        if (m_Pos.x < centerX - kCenterEps)      { nextX += std::min(speed, centerX - m_Pos.x); centering = true; }
        else if (m_Pos.x > centerX + kCenterEps) { nextX -= std::min(speed, m_Pos.x - centerX); centering = true; }
        if (m_Pos.y < centerY - kCenterEps)      { nextY += std::min(speed, centerY - m_Pos.y); centering = true; }
        else if (m_Pos.y > centerY + kCenterEps) { nextY -= std::min(speed, m_Pos.y - centerY); centering = true; }
    }

    if (moveX || moveY || isForced || centering) {  // 正在移動或正在歸位中心
        if (!IsColliding(nextX, m_Pos.y, worldContext)) {
            m_Pos.x = nextX;
        }
        if (!IsColliding(m_Pos.x, nextY, worldContext)) {
            m_Pos.y = nextY;
        }

        // 計算角色真正的座標
        m_GridX = GridCoord::ToGridX(m_Pos.x);
        m_GridY = GridCoord::ToGridY(m_Pos.y);

        if (!m_IgnoreBombs.empty()) {
            for (auto it = m_IgnoreBombs.begin(); it != m_IgnoreBombs.end(); ) {
                float bombPixelX = GridCoord::ToPixelX(it->first);
                float bombPixelY = GridCoord::ToPixelY(it->second);

                if (std::abs(m_Pos.x - bombPixelX) >= Constants::Player::kIgnoreBombClearance ||
                    std::abs(m_Pos.y - bombPixelY) >= Constants::Player::kIgnoreBombClearance) {
                    it = m_IgnoreBombs.erase(it);
                }
                else {
                    ++it;
                }
            }
        }
    }

    if (m_Bounce.IsPending()) {
        glm::vec2 center = GridCoord::ToPixel(m_GridX, m_GridY);
        m_Bounce.Begin(m_Pos, center,
            [&](glm::vec2 p) { return IsColliding(p.x, p.y, worldContext, /*ignoreBombs=*/true); });
        ChangeDirection(m_Bounce.PendingDir());
    }

    // 單次寫入畫面像素座標 (避免每幀重複寫兩次)
    m_Transform.translation = {
        std::round(m_Pos.x),
        std::round(m_Pos.y + Constants::Player::kSpriteYOffset)
    };
}

// 切換方向 — 方向→貼圖對映由 PlayerAnimator 提供 (取代 switch；新增方向不改本函式)
void Player::ChangeDirection(Direction dir) {
    if (m_CurrentDir == dir) return;  // 方向沒變
    m_CurrentDir = dir;
    SetDrawable(m_Animator.GetImage(dir));
}

bool Player::IsColliding(float nextX, float nextY, const IWorldContext& worldContext, bool ignoreBombs) {
    float radius = Constants::Player::kCollisionRadius;
    float left = nextX - radius;
    float right = nextX + radius;
    float top = nextY + radius;
    float bottom = nextY - radius;

    // 檢查四個角落的網格是否可走或有炸彈
    auto check = [&](int gx, int gy) {
        bool isMapObstacle = !worldContext.IsWalkable(gx, gy);
        bool isBomb        =  ignoreBombs ? false : worldContext.IsBombAt(gx, gy);  // 彈跳中可飛過炸彈
        bool isTurret      =  worldContext.IsTurretAt(gx, gy);

        // 玩家自己剛放的炸彈在 IgnoreBombs 中視為可穿透 (不擋玩家)
        bool isIgnored = false;
        for (const auto& ig : m_IgnoreBombs) {
            if (gx == ig.first && gy == ig.second) {
                isIgnored = true;
                break;
            }
        }
        if (isIgnored) {
            return isMapObstacle || isTurret;
        }

        return isMapObstacle || isBomb || isTurret;
    };

    if (check(GridCoord::ToGridX(left),  GridCoord::ToGridY(top)))    return true;
    if (check(GridCoord::ToGridX(right), GridCoord::ToGridY(top)))    return true;
    if (check(GridCoord::ToGridX(left),  GridCoord::ToGridY(bottom))) return true;
    if (check(GridCoord::ToGridX(right), GridCoord::ToGridY(bottom))) return true;

    return false;
}

// 重生角色
void Player::Respawn() {
    m_GridX = m_SpawnX;
    m_GridY = m_SpawnY;
    m_Pos = GridCoord::ToPixel(m_GridX, m_GridY);
    m_Transform.translation = { m_Pos.x, m_Pos.y + Constants::Player::kSpriteYOffset };
    m_Transform.rotation = 0.0f;
    m_CurrentBombs = 0;
    m_MaxBombs = Constants::Player::kInitialMaxBombs;
    m_Firepower = Constants::Player::kInitialFirepower;
    SetVisible(true);

    m_Lifecycle.OnRespawned();
}

void Player::Kill() {
    const auto outcome = m_Lifecycle.OnHit(m_Bounce.IsActive());
    if (outcome == PlayerLifecycle::HitOutcome::Immune) return;

    m_IgnoreBombs.clear();
    m_Bounce.Cancel();

    if (outcome == PlayerLifecycle::HitOutcome::KnockedDown) {
        LOG_INFO("Player knocked down! Lives left: " + std::to_string(GetLives()));
        return;
    }

    // Killed：死亡時若持有鑰匙就掉落，由 GameSession 放回世界
    if (m_HasKey) {
        m_HasKey = false;
        m_DroppedKeyPending = true;
    }
    LOG_INFO("Player died");
}

void Player::DebugKill() {
    const auto outcome = m_Lifecycle.OnDebugKill();
    if (outcome != PlayerLifecycle::HitOutcome::Killed) return;
    m_IgnoreBombs.clear();
    m_Bounce.Cancel();
    if (m_HasKey) {
        m_HasKey = false;
        m_DroppedKeyPending = true;
    }
    LOG_INFO("Player died");
}

void Player::IncreaseMaxBombs() {
    if (m_MaxBombs < Constants::Player::kMaxBombsCap) {
        m_MaxBombs++;
        LOG_INFO("Player's max bombs increased to " + std::to_string(m_MaxBombs));
    }
}

void Player::IncreaseFirepower() {
    if (m_Firepower < Constants::Player::kFirepowerCap) {
        m_Firepower++;
        LOG_INFO("Player's firepower increased to " + std::to_string(m_Firepower));
    }
}

bool Player::TriggerBounce(Direction dir, int distance) {
    return m_Bounce.Trigger(dir, distance);
}
