#include "Entities/PlayerAnimator.hpp"

PlayerAnimator::PlayerAnimator() {
    // 對應 Direction 列舉的順序 (UP, DOWN, LEFT, RIGHT)。
    m_Images[static_cast<int>(Direction::UP)]    = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/player_up.png");
    m_Images[static_cast<int>(Direction::DOWN)]  = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/player_down.png");
    m_Images[static_cast<int>(Direction::LEFT)]  = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/player_left.png");
    m_Images[static_cast<int>(Direction::RIGHT)] = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/player_right.png");
}
