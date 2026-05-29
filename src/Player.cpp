#include "Player.hpp"
#include "BombManager.hpp"
#include "GridCoord.hpp"
#include "InteractableManager.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"
#include "Util/Logger.hpp"
#include <cmath>
#include <algorithm>

Player::Player(int startGridX, int startGridY, Team team, std::unique_ptr<InputController> controller, int id) : m_GridX(startGridX), m_GridY(startGridY), m_SpawnX(startGridX), m_SpawnY(startGridY),
    m_CurrentDir(Direction::DOWN), m_Team(team), m_Controller(std::move(controller)), m_PlayerID(id) {

    m_ImgUp = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/player_up.png");
    m_ImgDown = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/player_down.png");
    m_ImgLeft = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/player_left.png");
    m_ImgRight = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/player_right.png");

    SetDrawable(m_ImgDown); // 初始面向下方
    SetZIndex(20);

    // 放大角色至設定的 sprite 尺寸
    m_Transform.scale = {
        Constants::Player::kSpriteWidth  / m_ImgDown->GetSize().x,
        Constants::Player::kSpriteHeight / m_ImgDown->GetSize().y
    };

    // 計算初始像素座標
    m_Pos = GridCoord::ToPixel(m_GridX, m_GridY);

    m_Transform.translation = { m_Pos.x, m_Pos.y + Constants::Player::kSpriteYOffset };
}

void Player::Update(const IWorldContext& worldContext) {
    float speed = Constants::Player::kNormalSpeed;
    float dx = 0.0f;
    float dy = 0.0f;
    float nextX = m_Pos.x;
    float nextY = m_Pos.y;

    if (m_SpeedBoostTimer > 0) {
        m_SpeedBoostTimer--;
        speed = Constants::Player::kBoostSpeed;
    }
    else {
        speed = Constants::Player::kNormalSpeed;
    }

    if (m_IsDead) {
        if (m_DeathCountdown > 0) {
            m_DeathCountdown--;
            if (m_DeathCountdown == 0) {
                SetVisible(false);
                m_RespawnTimer = Constants::Player::kRespawnDelayFrames;
                m_DeathCountdown = -1;
            }
        }
        else if (m_RespawnTimer > 0) {
            m_RespawnTimer--;
            if (m_RespawnTimer == 0) {
                Respawn();
                m_RespawnTimer = -1;
            }
        }
        return;
    }

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

    if (moveX || moveY || isForced) {  // 正在移動
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
            [&](glm::vec2 p) { return IsColliding(p.x, p.y, worldContext); });
        ChangeDirection(m_Bounce.PendingDir());
    }

    // 單次寫入畫面像素座標 (避免每幀重複寫兩次)
    m_Transform.translation = {
        std::round(m_Pos.x),
        std::round(m_Pos.y + Constants::Player::kSpriteYOffset)
    };

    // 無敵時間
    if (m_Invincible > 0) {
        m_Invincible--;
    }
}

// 切換方向
void Player::ChangeDirection(Direction dir) {
    if (m_CurrentDir == dir)
        return;  // 方向沒變

    m_CurrentDir = dir;
    switch (m_CurrentDir) {
        case Direction::UP:
            SetDrawable(m_ImgUp);
            break;
        case Direction::DOWN:
            SetDrawable(m_ImgDown);
            break;
        case Direction::LEFT:
            SetDrawable(m_ImgLeft);
            break;
        case Direction::RIGHT:
            SetDrawable(m_ImgRight);
            break;
    }
}

bool Player::IsColliding(float nextX, float nextY, const IWorldContext& worldContext) {
    float radius = Constants::Player::kCollisionRadius;
    float left = nextX - radius;
    float right = nextX + radius;
    float top = nextY + radius;
    float bottom = nextY - radius;

    // 檢查四個角落的網格是否可走或有炸彈
    auto check = [&](int gx, int gy) {
        bool isMapObstacle = !worldContext.IsWalkable(gx, gy);
        bool isBomb        =  worldContext.IsBombAt(gx, gy);
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
    m_IsDead = false;
    m_CurrentBombs = 0;
    m_MaxBombs = Constants::Player::kInitialMaxBombs;
    m_Firepower = Constants::Player::kInitialFirepower;
    SetVisible(true);

    m_Invincible = Constants::Player::kInvincibleFramesOnRespawn;
}

void Player::Kill() {
    if (m_Invincible > 0)  // 無敵時間
        return;

    m_IsDead = true;
    m_IgnoreBombs.clear();
    m_DeathCountdown = Constants::Player::kDeathCountdownFrames;

    // 死亡時若持有鑰匙就掉落：清掉持有狀態並標記待掉落，由 GameSession 放回世界
    if (m_HasKey) {
        m_HasKey = false;
        m_DroppedKeyPending = true;
    }

    // 確保死亡時清空彈跳狀態，避免重生後莫名其妙被彈走
    m_Bounce.Cancel();

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