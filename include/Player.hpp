#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "Util/GameObject.hpp"
#include "Util/Image.hpp"
#include <memory>
#include "glm/vec2.hpp"
#include "LevelManager.hpp"

class BombManager;

class Player : public Util::GameObject {
public:
    enum class Direction { UP, DOWN, LEFT, RIGHT };

    Player(int startGridX, int startGridY);

    // void Update(const LevelManager& levelManager);
    void Update(const LevelManager& levelManager, const class BombManager& bombManager);

    // 取得角色真正的座標
    int GetGridX() const { return m_GridX; }
    int GetGridY() const { return m_GridY; }

    bool IsDead() const { return m_IsDead; }
    void Kill() { 
        if (m_Invincible > 0)  // 無敵時間
            return;
        m_IsDead = true; 
    }

    void Respawn(int gridX, int gridY);

    void AddBombCount() { m_CurrentBombs++; }
    void DecBombCount() { m_CurrentBombs--; }
    bool CanPlaceBomb() const { return m_CurrentBombs < m_MaxBombs; }

	Direction GetDirection() const { return m_CurrentDir; }  // 取得當前方向

	void SetIgnoreBomb(int gx, int gy) { m_IgnoreBombX = gx; m_IgnoreBombY = gy; }  // 設定放置炸彈後暫時忽略的座標

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

    //bool IsColliding(float nextX, float nextY, const LevelManager& levelManager);  // 確認是否可以移動
    bool IsColliding(float nextX, float nextY, const LevelManager& levelManager, const class BombManager& bombManager);  // 確認是否可以移動

	bool m_IsDead = false;  // 角色是否死亡

    int m_MaxBombs = 3;        // 最大炸彈放置數量
    int m_CurrentBombs = 0;    // 當前場上炸彈數量

	// 暫時忽略的炸彈座標
    int m_IgnoreBombX = -1;
    int m_IgnoreBombY = -1;

    int m_Invincible = -1;  // 無敵時間
};

#endif