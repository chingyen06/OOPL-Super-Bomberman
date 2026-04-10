#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "Util/GameObject.hpp"
#include "Util/Image.hpp"
#include "Util/Keycode.hpp"
#include <memory>
#include <vector>
#include <utility>
#include "glm/vec2.hpp"
#include "LevelManager.hpp"

class BombManager;
class InteractableManager;

enum class Team {
    ATTACKER,
    DEFENDER
};

struct Control {
    Util::Keycode UP;
    Util::Keycode DOWN;
    Util::Keycode LEFT;
    Util::Keycode RIGHT;
    Util::Keycode PLACEBOMB;
};

class Player : public Util::GameObject {
public:
    enum class Direction { UP, DOWN, LEFT, RIGHT };

    // Player(int startGridX, int startGridY);
    Player(int startGridX, int startGridY, Team m_Team, Control m_Control, int id);

    // void Update(const LevelManager& levelManager);
    void Update(const LevelManager& levelManager, const class BombManager& bombManager, const InteractableManager& interactableManager);

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

    Util::Keycode GetBombKey() const { return m_Control.PLACEBOMB; }
	Team GetTeam() const { return m_Team; }

    int GetPlayerID() const { return m_PlayerID; };

    void ActivateSpeedBoost() { m_SpeedBoostTimer = 300;}

    // AI
    void SetBot(bool isBot) { m_IsBot = isBot; }
    bool IsBot() const { return m_IsBot; }
    void SetBotInput(bool up, bool down, bool left, bool right, bool placeBomb) {
        m_BotUp = up; m_BotDown = down; m_BotLeft = left; m_BotRight = right; m_BotPlaceBomb = placeBomb;
    }
    bool IsBotPlaceBomb() const { return m_BotPlaceBomb; }

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
    bool IsColliding(float nextX, float nextY, const LevelManager& levelManager, const class BombManager& bombManager, const InteractableManager& interactableManager);

	bool m_IsDead = false;  // 角色是否死亡

    int m_MaxBombs = 3;        // 最大炸彈放置數量
    int m_CurrentBombs = 0;    // 當前場上炸彈數量

	// 暫時忽略的炸彈座標
    std::vector<std::pair<int, int>> m_IgnoreBombs;

    int m_Invincible = -1;  // 無敵時間

    bool m_HasKey = false;  // 角色是否有鑰匙

    Team m_Team;
    Control m_Control;

    int m_SpawnX;
    int m_SpawnY;
    int m_DeathCountdown = -1;
    int m_RespawnTimer = -1;

    int m_PlayerID;

	int m_SpeedBoostTimer = -1;  // 速度提升計時器

    // AI
    bool m_IsBot = false;
    bool m_BotUp = false;
    bool m_BotDown = false;
    bool m_BotLeft = false;
    bool m_BotRight = false;
    bool m_BotPlaceBomb = false;
};

#endif