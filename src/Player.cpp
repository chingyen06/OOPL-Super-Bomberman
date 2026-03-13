#include "Player.hpp"
#include "BombManager.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"
#include "Util/Logger.hpp"
#include <cmath>

Player::Player(int startGridX, int startGridY) : m_GridX(startGridX), m_GridY(startGridY), m_CurrentDir(Direction::DOWN) {

    m_ImgUp = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/player_up.png");
    m_ImgDown = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/player_down.png");
    m_ImgLeft = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/player_left.png");
    m_ImgRight = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/player_right.png");

    SetDrawable(m_ImgDown); // 初始面向下方
    SetZIndex(10);

    // 放大角色：寬度 36、高度 48
    m_Transform.scale = { 36.0f / m_ImgDown->GetSize().x, 48.0f / m_ImgDown->GetSize().y };

    // 計算初始像素座標
    m_Pos.x = (m_GridX - 12) * 32.0f;
    m_Pos.y = (8 - m_GridY) * 32.0f;

    m_Transform.translation = { m_Pos.x, m_Pos.y };
}

void Player::Update(const LevelManager& levelManager, const class BombManager& bombManager) {
    float speed = 3.0f;  // 移動速度
    float dx = 0.0f;  // x 方向移動
    float dy = 0.0f;  // y 方向移動
    float nextX = m_Pos.x;
    float nextY = m_Pos.y;
    //bool moved = false;

    // 取得當前位置的網格中心點座標
    float centerX = (m_GridX - 12) * 32.0f;
    float centerY = (8 - m_GridY) * 32.0f;

    if (Util::Input::IsKeyPressed(Util::Keycode::W)) {
        dy += speed;
        ChangeDirection(Direction::UP);
    }
    else if (Util::Input::IsKeyPressed(Util::Keycode::S)) {
        dy -= speed;
        ChangeDirection(Direction::DOWN);
    }

    if (Util::Input::IsKeyPressed(Util::Keycode::A)) {
        dx -= speed;
        ChangeDirection(Direction::LEFT);
    }
    else if (Util::Input::IsKeyPressed(Util::Keycode::D)) {
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

    // 只有單一方向移動才做轉彎輔助對齊
    if (moveY && !moveX) {
        if (m_Pos.x < centerX) 
            nextX += std::min(speed, centerX - m_Pos.x);
        else if (m_Pos.x > centerX) 
            nextX -= std::min(speed, m_Pos.x - centerX);
    }
    else if (moveX && !moveY) {
        if (m_Pos.y < centerY) 
            nextY += std::min(speed, centerY - m_Pos.y);
        else if (m_Pos.y > centerY) 
            nextY -= std::min(speed, m_Pos.y - centerY);
    }

    if (moveX || moveY) {  // 正在移動
        if (!IsColliding(nextX, m_Pos.y, levelManager, bombManager)) {  // 如果不會碰撞，可以移動
            m_Pos.x = nextX;
        }
        if (!IsColliding(m_Pos.x, nextY, levelManager, bombManager)) {  // 如果不會碰撞，可以移動
            m_Pos.y = nextY;
        }

        m_Transform.translation = { m_Pos.x, m_Pos.y + 8.0f };  // 更新畫面像素座標

        // 計算角色真正的座標
        m_GridX = std::round(m_Pos.x / 32.0f) + 12;
        m_GridY = 8 - std::round(m_Pos.y / 32.0f);

		/*if (m_IgnoreBombX != -1) {  // 如果正在忽略炸彈，檢查是否已經離開那個格子
            // 取角色中心點所在的網格
            int currentGX = std::round(m_Pos.x / 32.0f) + 12;
            int currentGY = 8 - std::round(m_Pos.y / 32.0f);

            // 如果中心點已經不在放炸彈的那一格，就恢復卡位判定
            if (currentGX != m_IgnoreBombX || currentGY != m_IgnoreBombY) {
                m_IgnoreBombX = -1;
                m_IgnoreBombY = -1;
            }
        }*/

        if (m_IgnoreBombX != -1) {  // 如果正在忽略炸彈，檢查是否已經離開那個格子
            // 取得目前碰撞箱的四個邊界所在的網格座標
            float radius = 9.0f;
            auto getGX = [](float x) { return static_cast<int>(std::floor((x + 16.0f) / 32.0f)) + 12; };
            auto getGY = [](float y) { return 8 - static_cast<int>(std::floor((y + 16.0f) / 32.0f)); };

            int gxl = getGX(m_Pos.x - radius);
            int gxr = getGX(m_Pos.x + radius);
            int gyt = getGY(m_Pos.y + radius);
            int gyb = getGY(m_Pos.y - radius);

            // 檢查有沒有任何角落還在被忽略的炸彈格內
            bool isStillTouching = false;
            if ((gxl == m_IgnoreBombX || gxr == m_IgnoreBombX) &&
                (gyt == m_IgnoreBombY || gyb == m_IgnoreBombY)) {
                isStillTouching = true;
            }

            // 只有當四個角落都完全離開了該格子，才恢復卡位
            if (!isStillTouching) {
                m_IgnoreBombX = -1;
                m_IgnoreBombY = -1;
                // LOG_INFO("Successfully exited bomb at (" + std::to_string(m_IgnoreBombX) + ")");
            }
        }
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

bool Player::IsColliding(float nextX, float nextY, const LevelManager& levelManager, const class BombManager& bombManager) {
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
        // return !levelManager.IsWalkable(gx, gy) || bombManager.IsBombAt(gx, gy);
        bool isMapObstacle = !levelManager.IsWalkable(gx, gy);
        bool isBomb = bombManager.IsBombAt(gx, gy);

        // 如果是炸彈，但該座標等於 m_IgnoreBombX/Y，則視為可穿透
        if (gx == m_IgnoreBombX && gy == m_IgnoreBombY) {
            return isMapObstacle;
        }
        return isMapObstacle || isBomb;
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
void Player::Respawn(int gridX, int gridY) {
    m_GridX = gridX;
    m_GridY = gridY;
    m_Pos.x = (m_GridX - 12) * 32.0f;
    m_Pos.y = (8 - m_GridY) * 32.0f;
    m_Transform.translation = { m_Pos.x, m_Pos.y + 8.0f };
    m_IsDead = false;
    m_CurrentBombs = 0;
}