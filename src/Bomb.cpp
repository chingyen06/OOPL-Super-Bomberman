#include "Bomb.hpp"
#include "LevelManager.hpp"
#include "BombManager.hpp"
#include "InteractableManager.hpp"

Bomb::Bomb(int gridX, int gridY, int firepower, int ownerID) : m_GridX(gridX), m_GridY(gridY), m_Firepower(firepower),
           m_Tick(180), m_State(State::COUNTDOWN), m_OwnerID(ownerID) {
    auto image = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/bomb.png");
    SetDrawable(image);
    SetZIndex(4); // 草地上，主角與牆壁下方

    m_Pos.x = (gridX - 12) * 32.0f;
    m_Pos.y = (8 - gridY) * 32.0f;

    m_Transform.scale = { 32.0f / image->GetSize().x, 32.0f / image->GetSize().y };
    m_Transform.translation = { (gridX - 12) * 32.0f, (8 - gridY) * 32.0f };
}

void Bomb::Update(const LevelManager& levelManager, const BombManager& bombManager, const InteractableManager& interactableManager) {
    if (m_State == State::COUNTDOWN) {
        m_Tick--;
        if (m_Tick <= 0) m_State = State::DONE;
    }

    glm::vec2 force = interactableManager.GetForceAt(m_GridX, m_GridY);
    float centerX = (m_GridX - 12) * 32.0f;
    float centerY = (8 - m_GridY) * 32.0f;

    if (force.x == 0.0f && force.y == 0.0f) {
        float alignSpeed = 1.5f;
        if (m_Pos.x < centerX) force.x = std::min(alignSpeed, centerX - m_Pos.x);
        else if (m_Pos.x > centerX) force.x = -std::min(alignSpeed, m_Pos.x - centerX);

        if (m_Pos.y < centerY) force.y = std::min(alignSpeed, centerY - m_Pos.y);
        else if (m_Pos.y > centerY) force.y = -std::min(alignSpeed, m_Pos.y - centerY);
    }

    // 只要有力量 (地形推力 或 慣性)，就執行位移
    if (force.x != 0.0f || force.y != 0.0f) {
        float nextX = m_Pos.x + force.x;
        float nextY = m_Pos.y + force.y;

        float centerSpeed = 3.0f;
        if (force.x != 0.0f && force.y == 0.0f) {
            if (m_Pos.y < centerY) nextY += std::min(centerSpeed, centerY - m_Pos.y);
            else if (m_Pos.y > centerY) nextY -= std::min(centerSpeed, m_Pos.y - centerY);
        }
        else if (force.y != 0.0f && force.x == 0.0f) {
            if (m_Pos.x < centerX) nextX += std::min(centerSpeed, centerX - m_Pos.x);
            else if (m_Pos.x > centerX) nextX -= std::min(centerSpeed, m_Pos.x - centerX);
        }

        // 碰撞偵測
        auto checkCollision = [&](float nx, float ny, float dx, float dy) {
            float radius = 15.0f;
            auto getGX = [](float x) { return static_cast<int>(std::floor((x + 16.0f) / 32.0f)) + 12; };
            auto getGY = [](float y) { return 8 - static_cast<int>(std::floor((y + 16.0f) / 32.0f)); };

            auto checkGrid = [&](int gx, int gy) {
                if (!levelManager.IsWalkable(gx, gy)) return true;
                if (bombManager.IsBombAt(gx, gy, this)) return true;
                return false;
                };

            // 往右走，只檢查右邊的上下兩個角
            if (dx > 0 && (checkGrid(getGX(nx + radius), getGY(ny + radius - 2.0f)) || checkGrid(getGX(nx + radius), getGY(ny - radius + 2.0f)))) 
                return true;

            // 往左走，只檢查左邊的上下兩個角
            if (dx < 0 && (checkGrid(getGX(nx - radius), getGY(ny + radius - 2.0f)) || checkGrid(getGX(nx - radius), getGY(ny - radius + 2.0f)))) 
                return true;

            // 往上走，只檢查上面的左右兩個角
            if (dy > 0 && (checkGrid(getGX(nx + radius - 2.0f), getGY(ny + radius)) || checkGrid(getGX(nx - radius + 2.0f), getGY(ny + radius)))) 
                return true;

            // 往下走，只檢查下面的左右兩個角
            if (dy < 0 && (checkGrid(getGX(nx + radius - 2.0f), getGY(ny - radius)) || checkGrid(getGX(nx - radius + 2.0f), getGY(ny - radius)))) 
                return true;

            return false;
            };

        float moveX = nextX - m_Pos.x;
        float moveY = nextY - m_Pos.y;

        if (moveX != 0.0f && !checkCollision(nextX, m_Pos.y, moveX, 0.0f)) 
            m_Pos.x = nextX;
        if (moveY != 0.0f && !checkCollision(m_Pos.x, nextY, 0.0f, moveY))
            m_Pos.y = nextY;

        m_Transform.translation = { m_Pos.x, m_Pos.y };
        m_GridX = std::round(m_Pos.x / 32.0f) + 12;
        m_GridY = 8 - std::round(m_Pos.y / 32.0f);
    }
}