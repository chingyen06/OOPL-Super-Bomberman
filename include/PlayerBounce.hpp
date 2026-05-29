#ifndef PLAYER_BOUNCE_HPP
#define PLAYER_BOUNCE_HPP

#include <functional>
#include "glm/vec2.hpp"
#include "GameTypes.hpp"
#include "GameConstants.hpp"

// 彈跳板的飛行動畫狀態機 — 從 Player 抽出 (SRP)。
// 本身不認識 Player 或地圖碰撞：Begin 時由呼叫端注入「某像素點能否落腳」的判斷，
// Player 只負責把回傳的邏輯座標寫進畫面 transform 與網格座標。
class PlayerBounce {
public:
    bool IsActive() const { return m_Active; }
    bool IsPending() const { return m_Pending; }
    Direction PendingDir() const { return m_PendingDir; }

    // 由 BouncePad 觸發；已在彈跳/待發中則回 false 不重複觸發
    bool Trigger(Direction dir, int distance);

    // 把「待發」轉成實際彈跳：沿 dir 逐格試探最遠可落腳點 (以 collide 判斷)。
    // currentPos: 目前邏輯像素座標 (彈跳起點)；centerPos: 目前所在格中心像素。
    void Begin(glm::vec2 currentPos, glm::vec2 centerPos,
               const std::function<bool(glm::vec2)>& collide);

    // 推進一幀。pos = 本幀邏輯像素座標；jumpHeight = 視覺拋物線高度；
    // finished = 本幀抵達終點 (Player 應改用無跳躍高度的 transform 並重算網格)。
    struct Step { glm::vec2 pos; float jumpHeight; bool finished; };
    Step Update();

    void Cancel() { *this = PlayerBounce{}; }

private:
    bool m_Active = false;
    bool m_Pending = false;
    int m_Tick = 0;
    int m_Duration = Constants::Player::kBounceFrames;
    glm::vec2 m_Start{};
    glm::vec2 m_Target{};
    Direction m_PendingDir = Direction::DOWN;
    int m_PendingDist = 0;
};

#endif
