#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "Util/GameObject.hpp"
#include "Util/Image.hpp"
#include "Util/Keycode.hpp"
#include <memory>
#include <vector>
#include <utility>
#include "glm/vec2.hpp"
#include "WorldContext.hpp"
#include "Controller/InputController.hpp"

enum class Team {
    ATTACKER,
    DEFENDER
};

class Player : public Util::GameObject {
public:
    enum class Direction { UP, DOWN, LEFT, RIGHT };

    // Player(int startGridX, int startGridY);
    Player(int startGridX, int startGridY, Team m_Team, std::unique_ptr<InputController> controller, int id);

    // void Update(const LevelManager& levelManager);
    void Update(const IWorldContext& worldContext);

    // 取得角色真正的座標
    int GetGridX() const { return m_GridX; }
    int GetGridY() const { return m_GridY; }

    // 取得角色像素座標
    glm::vec2 GetPixelPos() const { return m_Pos; }

    bool IsDead() const { return m_IsDead; }
    void Kill();

    // void Respawn(int gridX, int gridY
    void Respawn();

    void AddBombCount() { m_CurrentBombs++; }
    void DecBombCount() { m_CurrentBombs--; }
    bool CanPlaceBomb() const { return m_CurrentBombs < m_MaxBombs; }

	Direction GetDirection() const { return m_CurrentDir; }  // 取得當前方向

	void SetIgnoreBomb(int gx, int gy) { m_IgnoreBombs.push_back({ gx, gy }); }  // 設定放置炸彈後暫時忽略的座標

    bool HasKey() const { return m_HasKey; }
    void SetKey(bool key) { m_HasKey = key; }

	Team GetTeam() const { return m_Team; }

    int GetPlayerID() const { return m_PlayerID; };

    void ActivateSpeedBoost() { m_SpeedBoostTimer = 300;}
    void IncreaseMaxBombs();
    int GetFirepower() const { return m_Firepower; }
    void IncreaseFirepower();

    InputController* GetController() const { return m_Controller.get(); }

    bool TriggerBounce(Direction dir, int distance);  // 彈跳板

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

    // 確認是否可以移動
    bool IsColliding(float nextX, float nextY, const IWorldContext& worldContext);

	bool m_IsDead = false;  // 角色是否死亡

    int m_MaxBombs = 3;        // 最大炸彈放置數量
    int m_CurrentBombs = 0;    // 當前場上炸彈數量

	// 暫時忽略的炸彈座標
    std::vector<std::pair<int, int>> m_IgnoreBombs;

    int m_Invincible = -1;  // 無敵時間

    bool m_HasKey = false;  // 角色是否有鑰匙

    Team m_Team;
    std::unique_ptr<InputController> m_Controller;

    int m_SpawnX;
    int m_SpawnY;
    int m_DeathCountdown = -1;
    int m_RespawnTimer = -1;

    int m_PlayerID;

	int m_SpeedBoostTimer = -1;  // 速度提升計時器

	int m_Firepower = 2;  // 炸彈火力，初始為 2，最大為 5

    struct BounceState {
        bool active = false;
        bool pending = false;
        int tick = 0;
        int duration = 30;
        glm::vec2 start{};
        glm::vec2 target{};
        Direction pendingDir;
        int pendingDist = 0;
    };
    BounceState m_Bounce;

    void UpdateBouncing();
    void ApplyPendingBounce(const IWorldContext& context);
};

#endif