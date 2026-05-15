#include "Player.hpp"
#include "BombManager.hpp"
#include "InteractableManager.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"
#include "Util/Logger.hpp"
#include <cmath>

Player::Player(int startGridX, int startGridY, Team team, std::unique_ptr<InputController> controller, int id) : m_GridX(startGridX), m_GridY(startGridY), m_SpawnX(startGridX), m_SpawnY(startGridY),
    m_CurrentDir(Direction::DOWN), m_Team(team), m_Controller(std::move(controller)), m_PlayerID(id) {

    m_ImgUp = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/player_up.png");
    m_ImgDown = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/player_down.png");
    m_ImgLeft = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/player_left.png");
    m_ImgRight = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/player_right.png");

    SetDrawable(m_ImgDown); // 初始面向下方
    SetZIndex(20);

    // 放大角色：寬度 36、高度 48
    m_Transform.scale = { 36.0f / m_ImgDown->GetSize().x, 48.0f / m_ImgDown->GetSize().y };

    // 計算初始像素座標
    m_Pos.x = (m_GridX - 12) * 32.0f;
    m_Pos.y = (8 - m_GridY) * 32.0f;

    m_Transform.translation = { m_Pos.x, m_Pos.y + 15.0f };
}

void Player::Update(const IWorldContext& worldContext) {
    float speed = 3.0f;
    float dx = 0.0f;
    float dy = 0.0f;
    float nextX = m_Pos.x;
    float nextY = m_Pos.y;
    //bool moved = false;

    if (m_SpeedBoostTimer > 0) {
        m_SpeedBoostTimer--;
		speed = 5.0f;  // 速度提升
    }
    else {
		speed = 3.0f;
    }

    if (m_IsDead) {
        if (m_DeathCountdown > 0) {
            m_DeathCountdown--;
            if (m_DeathCountdown == 0) {
                SetVisible(false);
                m_RespawnTimer = 90;
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

    if (m_Bounce.active) {
        UpdateBouncing();
        return;
    }

    // 取得當前位置的網格中心點座標
    float centerX = (m_GridX - 12) * 32.0f;
    float centerY = (8 - m_GridY) * 32.0f;

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
        if (!IsColliding(nextX, m_Pos.y, worldContext)) {  // 如果不會碰撞，可以移動
            m_Pos.x = nextX;
        }
        if (!IsColliding(m_Pos.x, nextY, worldContext)) {  // 如果不會碰撞，可以移動
            m_Pos.y = nextY;
        }

        m_Transform.translation = { m_Pos.x, m_Pos.y + 15.0f };  // 更新畫面像素座標

        // 計算角色真正的座標
        m_GridX = std::round(m_Pos.x / 32.0f) + 12;
        m_GridY = 8 - std::round(m_Pos.y / 32.0f);

        if (!m_IgnoreBombs.empty()) {
            for (auto it = m_IgnoreBombs.begin(); it != m_IgnoreBombs.end(); ) {
                float bombPixelX = (it->first - 12) * 32.0f;
                float bombPixelY = (8 - it->second) * 32.0f;

                if (std::abs(m_Pos.x - bombPixelX) >= 40.0f || std::abs(m_Pos.y - bombPixelY) >= 40.0f) {
                    it = m_IgnoreBombs.erase(it);
                }
                else {
                    ++it;
                }
            }
        }
    }

    if (m_Bounce.pending) {
        ApplyPendingBounce(worldContext);
    }

	// 位置更新後，確保畫面座標與像素座標同步
    m_Transform.translation = {
        std::round(m_Pos.x),
        std::round(m_Pos.y + 15.0f)
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
    float radius = 9.0f;  // 碰撞箱半徑
    float left = nextX - radius;
    float right = nextX + radius;
    float top = nextY + radius;
    float bottom = nextY - radius;

    // 用像素座標計算真正的網格座標
    auto getGridX = [](float x) { return static_cast<int>(std::floor((x + 16.0f) / 32.0f)) + 12; };
    auto getGridY = [](float y) { return 8 - static_cast<int>(std::floor((y + 16.0f) / 32.0f)); };

	// 檢查四個角落的網格座標是否可行走或有炸彈
    auto check = [&](int gx, int gy) {
        bool isMapObstacle = !worldContext.IsWalkable(gx, gy);
        bool isBomb = worldContext.IsBombAt(gx, gy);
        bool isTurret = worldContext.IsTurretAt(gx, gy);
        // bool isChest = interactableManager.IsChestAt(gx, gy);

        // 如果是炸彈，但該座標等於 m_IgnoreBombX/Y，則視為可穿透
        /*if (gx == m_IgnoreBombX && gy == m_IgnoreBombY) {
            return isMapObstacle;
        }*/
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

    // 其中一個角不能走就判定有碰撞
    if (check(getGridX(left), getGridY(top)))
        return true;
    if (check(getGridX(right), getGridY(top)))
        return true;
    if (check(getGridX(left), getGridY(bottom)))
        return true;
    if (check(getGridX(right), getGridY(bottom)))
        return true;

    return false;  // 無碰撞
}

// 重生角色
void Player::Respawn() {
	m_GridX = m_SpawnX;
	m_GridY = m_SpawnY;
    m_Pos.x = (m_GridX - 12) * 32.0f;
    m_Pos.y = (8 - m_GridY) * 32.0f;
    m_Transform.translation = { m_Pos.x, m_Pos.y + 15.0f };
    m_IsDead = false;
    m_CurrentBombs = 0;
    m_MaxBombs = 3;
    m_Firepower = 2;
    SetVisible(true);

    m_Invincible = 180;  // 無敵時間
}

void Player::Kill() {
    if (m_Invincible > 0)  // 無敵時間
        return;

    m_IsDead = true;
    m_IgnoreBombs.clear();
    m_DeathCountdown = 30;

    LOG_INFO("Player died");
}

void Player::IncreaseMaxBombs() {
    if (m_MaxBombs < 10) {
        m_MaxBombs++;
        LOG_INFO("Player's max bombs increased to " + std::to_string(m_MaxBombs));
	}
}

void Player::IncreaseFirepower() {
    if (m_Firepower < 5) {
        m_Firepower++;
        LOG_INFO("Player's firepower increased to " + std::to_string(m_Firepower));
	}
}

bool Player::TriggerBounce(Direction dir, int distance) {
    if (m_Bounce.active || m_Bounce.pending) return false;

    m_Bounce.pending = true;
    m_Bounce.pendingDir = dir;
    m_Bounce.pendingDist = distance;
    return true;
}

void Player::UpdateBouncing() {
    m_Bounce.tick++;
    float t = static_cast<float>(m_Bounce.tick) / m_Bounce.duration;

    m_Pos.x = m_Bounce.start.x + (m_Bounce.target.x - m_Bounce.start.x) * t;
    m_Pos.y = m_Bounce.start.y + (m_Bounce.target.y - m_Bounce.start.y) * t;

    float jumpHeight = std::sin(t * 3.14159f) * 64.0f;
    m_Transform.translation = { m_Pos.x, m_Pos.y + 15.0f + jumpHeight };

    if (m_Bounce.tick >= m_Bounce.duration) {
        m_Bounce.active = false;
        m_Pos = m_Bounce.target;

        m_GridX = std::round(m_Pos.x / 32.0f) + 12;
        m_GridY = 8 - std::round(m_Pos.y / 32.0f);
        m_Transform.translation = { m_Pos.x, m_Pos.y + 15.0f };
    }
}

void Player::ApplyPendingBounce(const GameWorldContext& worldContext) {
    m_Bounce.pending = false;

    float centerX = (m_GridX - 12) * 32.0f;
    float centerY = (8 - m_GridY) * 32.0f;

    float bdx = 0.0f, bdy = 0.0f;
    switch (m_Bounce.pendingDir) {
    case Direction::UP:    bdy = 1.0f;  break;
    case Direction::DOWN:  bdy = -1.0f; break;
    case Direction::LEFT:  bdx = -1.0f; break;
    case Direction::RIGHT: bdx = 1.0f;  break;
    }

    int actualDist = 0;
    for (int i = 1; i <= m_Bounce.pendingDist; i++) {
        float testX = centerX + bdx * i * 32.0f;
        float testY = centerY + bdy * i * 32.0f;

        if (!IsColliding(testX, testY, worldContext)) {
            actualDist = i;
        }
        else {
            break;
        }
    }

    m_Bounce.active = true;
    m_Bounce.tick = 0;
    m_Bounce.start = m_Pos;
    ChangeDirection(m_Bounce.pendingDir);

    if (actualDist > 0) {
        m_Bounce.target = { centerX + bdx * actualDist * 32.0f, centerY + bdy * actualDist * 32.0f };
        m_Bounce.duration = 30;
    }
    else {
        m_Bounce.target = { centerX, centerY };
        m_Bounce.duration = 15;
        LOG_INFO("Bounce path blocked, bouncing in place.");
    }
}