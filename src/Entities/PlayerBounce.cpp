#include "PlayerBounce.hpp"
#include <cmath>
#include "GridCoord.hpp"
#include "Util/Logger.hpp"

bool PlayerBounce::Trigger(Direction dir, int distance) {
    if (m_Active || m_Pending) return false;
    m_Pending = true;
    m_PendingDir = dir;
    m_PendingDist = distance;
    return true;
}

void PlayerBounce::Begin(glm::vec2 currentPos, glm::vec2 centerPos,
                         const std::function<bool(glm::vec2)>& collide) {
    m_Pending = false;

    float bdx = 0.0f, bdy = 0.0f;
    switch (m_PendingDir) {
        case Direction::UP:    bdy = 1.0f;  break;
        case Direction::DOWN:  bdy = -1.0f; break;
        case Direction::LEFT:  bdx = -1.0f; break;
        case Direction::RIGHT: bdx = 1.0f;  break;
    }

    // 沿彈跳方向逐格試探，記下最遠可落腳的距離
    int actualDist = 0;
    for (int i = 1; i <= m_PendingDist; i++) {
        glm::vec2 test = { centerPos.x + bdx * i * GridCoord::kTileSize,
                           centerPos.y + bdy * i * GridCoord::kTileSize };
        if (!collide(test)) {
            actualDist = i;
        }
        else {
            break;
        }
    }

    m_Active = true;
    m_Tick = 0;
    m_Start = currentPos;

    if (actualDist > 0) {
        m_Target = { centerPos.x + bdx * actualDist * GridCoord::kTileSize,
                     centerPos.y + bdy * actualDist * GridCoord::kTileSize };
        m_Duration = Constants::Player::kBounceFrames;
    }
    else {
        m_Target = centerPos;
        m_Duration = Constants::Player::kBounceBlockedFrames;
        LOG_INFO("Bounce path blocked, bouncing in place.");
    }
}

PlayerBounce::Step PlayerBounce::Update() {
    m_Tick++;
    float t = static_cast<float>(m_Tick) / m_Duration;

    glm::vec2 pos = {
        m_Start.x + (m_Target.x - m_Start.x) * t,
        m_Start.y + (m_Target.y - m_Start.y) * t
    };
    float jumpHeight = std::sin(t * 3.14159f) * Constants::Player::kBounceJumpHeight;

    bool finished = (m_Tick >= m_Duration);
    if (finished) {
        m_Active = false;
        pos = m_Target;
    }
    return { pos, jumpHeight, finished };
}
