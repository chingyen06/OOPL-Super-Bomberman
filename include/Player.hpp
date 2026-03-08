#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "Util/GameObject.hpp"
#include "Util/Image.hpp"
#include <memory>
#include "glm/vec2.hpp"
#include "LevelManager.hpp"

class Player : public Util::GameObject {
public:
    enum class Direction { UP, DOWN, LEFT, RIGHT };

    Player(int startGridX, int startGridY);

    void Update(const LevelManager& levelManager);

    // 取得角色真正的座標
    int GetGridX() const { return m_GridX; }
    int GetGridY() const { return m_GridY; }

private:
    int m_GridX;
    int m_GridY;

    glm::vec2 m_Pos;  // 角色像素座標

    Direction m_CurrentDir;

    std::shared_ptr<Util::Image> m_ImgUp;
    std::shared_ptr<Util::Image> m_ImgDown;
    std::shared_ptr<Util::Image> m_ImgLeft;
    std::shared_ptr<Util::Image> m_ImgRight;

    void ChangeDirection(Direction dir);

    bool IsColliding(float nextX, float nextY, const LevelManager& levelManager);  // 確認是否可以移動
};

#endif