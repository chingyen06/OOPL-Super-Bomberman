#ifndef PLAYER_ANIMATOR_HPP
#define PLAYER_ANIMATOR_HPP

#include <array>
#include <memory>
#include "Util/Image.hpp"
#include "GameTypes.hpp"

// 玩家四方向貼圖管理 — 從 Player 拆出 (SRP)。
// 以陣列取代過去的 switch — 新增方向只要動 Direction 列舉與圖檔，
// Player::Update 不必再改 (OCP)。
class PlayerAnimator {
public:
    PlayerAnimator();

    // 取得指定方向的貼圖 (建立後不變動)。
    std::shared_ptr<Util::Image> GetImage(Direction dir) const {
        return m_Images[static_cast<int>(dir)];
    }

    // 取得初始貼圖大小，供 Player 計算 sprite 縮放。
    std::shared_ptr<Util::Image> InitialImage() const { return GetImage(Direction::DOWN); }

private:
    std::array<std::shared_ptr<Util::Image>, 4> m_Images;
};

#endif
