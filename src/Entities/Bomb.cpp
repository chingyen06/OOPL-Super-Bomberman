#include "Bomb.hpp"
#include "GameConstants.hpp"
#include "GridCoord.hpp"
#include "LevelManager.hpp"
#include "BombManager.hpp"
#include "InteractableManager.hpp"
#include <cmath>
#include <algorithm>

Bomb::Bomb(int gridX, int gridY, int firepower, int ownerID) : m_GridX(gridX), m_GridY(gridY), m_Firepower(firepower),
           m_Tick(Constants::Bomb::kFuseFrames), m_State(State::COUNTDOWN), m_OwnerID(ownerID) {
    auto image = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/bomb.png");
    SetDrawable(image);
    SetZIndex(4); // 在地板上，道具與火焰下方

    m_Pos = GridCoord::ToPixel(gridX, gridY);

    m_Transform.scale = { GridCoord::kTileSize / image->GetSize().x, GridCoord::kTileSize / image->GetSize().y };
    m_Transform.translation = m_Pos;
}

void Bomb::Update(const LevelManager& levelManager, const BombManager& bombManager, const InteractableManager& interactableManager) {
    if (m_State == State::COUNTDOWN) {
        m_Tick--;
        if (m_Tick <= 0) m_State = State::DONE;
    }

    // 已經 DONE 的炸彈當幀就會被 BombManager 處理，不需要再計算位移
    if (m_State == State::DONE) {
        return;
    }

    glm::vec2 force = interactableManager.GetForceAt(m_GridX, m_GridY);
    float centerX = GridCoord::ToPixelX(m_GridX);
    float centerY = GridCoord::ToPixelY(m_GridY);

    if (force.x == 0.0f && force.y == 0.0f) {
        constexpr float alignSpeed = Constants::Bomb::kAlignSpeed;
        if (m_Pos.x < centerX) force.x = std::min(alignSpeed, centerX - m_Pos.x);
        else if (m_Pos.x > centerX) force.x = -std::min(alignSpeed, m_Pos.x - centerX);

        if (m_Pos.y < centerY) force.y = std::min(alignSpeed, centerY - m_Pos.y);
        else if (m_Pos.y > centerY) force.y = -std::min(alignSpeed, m_Pos.y - centerY);
    }

    // Only move when there is force (conveyor or auto-alignment to tile centre)
    if (force.x != 0.0f || force.y != 0.0f) {
        float nextX = m_Pos.x + force.x;
        float nextY = m_Pos.y + force.y;

        constexpr float centerSpeed = Constants::Bomb::kCenterSpeed;
        if (force.x != 0.0f && force.y == 0.0f) {
            if (m_Pos.y < centerY) nextY += std::min(centerSpeed, centerY - m_Pos.y);
            else if (m_Pos.y > centerY) nextY -= std::min(centerSpeed, m_Pos.y - centerY);
        }
        else if (force.y != 0.0f && force.x == 0.0f) {
            if (m_Pos.x < centerX) nextX += std::min(centerSpeed, centerX - m_Pos.x);
            else if (m_Pos.x > centerX) nextX -= std::min(centerSpeed, m_Pos.x - centerX);
        }

        // 碰撞檢查
        auto checkCollision = [&](float nx, float ny, float dx, float dy) {
            constexpr float radius = Constants::Bomb::kCollisionRadius;

            auto checkGrid = [&](int gx, int gy) {
                if (!levelManager.IsWalkable(gx, gy)) return true;
                // 自己當前所在格永遠視為可走 — PlaceBomb 已保證同格不會有第二顆炸彈
                if (gx == m_GridX && gy == m_GridY) return false;
                if (bombManager.IsBombAt(gx, gy)) return true;
                return false;
            };

            // Moving right: only inspect the two corners on the right edge
            if (dx > 0 && (checkGrid(GridCoord::ToGridX(nx + radius), GridCoord::ToGridY(ny + radius - 2.0f)) ||
                           checkGrid(GridCoord::ToGridX(nx + radius), GridCoord::ToGridY(ny - radius + 2.0f))))
                return true;

            // Moving left
            if (dx < 0 && (checkGrid(GridCoord::ToGridX(nx - radius), GridCoord::ToGridY(ny + radius - 2.0f)) ||
                           checkGrid(GridCoord::ToGridX(nx - radius), GridCoord::ToGridY(ny - radius + 2.0f))))
                return true;

            // Moving up
            if (dy > 0 && (checkGrid(GridCoord::ToGridX(nx + radius - 2.0f), GridCoord::ToGridY(ny + radius)) ||
                           checkGrid(GridCoord::ToGridX(nx - radius + 2.0f), GridCoord::ToGridY(ny + radius))))
                return true;

            // Moving down
            if (dy < 0 && (checkGrid(GridCoord::ToGridX(nx + radius - 2.0f), GridCoord::ToGridY(ny - radius)) ||
                           checkGrid(GridCoord::ToGridX(nx - radius + 2.0f), GridCoord::ToGridY(ny - radius))))
                return true;

            return false;
        };

        float moveX = nextX - m_Pos.x;
        float moveY = nextY - m_Pos.y;

        if (moveX != 0.0f && !checkCollision(nextX, m_Pos.y, moveX, 0.0f)) 
            m_Pos.x = nextX;
        if (moveY != 0.0f && !checkCollision(m_Pos.x, nextY, 0.0f, moveY))
            m_Pos.y = nextY;

        m_Transform.translation = m_Pos;
        m_GridX = GridCoord::ToGridX(m_Pos.x);
        m_GridY = GridCoord::ToGridY(m_Pos.y);
    }
}