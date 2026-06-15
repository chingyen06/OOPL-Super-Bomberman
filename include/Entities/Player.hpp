#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "Util/GameObject.hpp"
#include <memory>
#include <vector>
#include <utility>
#include "glm/vec2.hpp"
#include "GameConstants.hpp"
#include "GameTypes.hpp"
#include "WorldContext.hpp"
#include "Controller/InputController.hpp"
#include "PlayerBounce.hpp"
#include "PlayerAnimator.hpp"
#include "PlayerLifecycle.hpp"

class Player : public Util::GameObject {
public:
    Player(int startGridX, int startGridY, Team team, std::unique_ptr<InputController> controller, int id);

    void Update(const IWorldContext& worldContext);

    // 取得角色真正的座標
    int GetGridX() const { return m_GridX; }
    int GetGridY() const { return m_GridY; }

    // 取得角色像素座標
    glm::vec2 GetPixelPos() const { return m_Pos; }

    // ---- Lifecycle 轉發 (對外 API 不變，內部委派給 PlayerLifecycle) ----
    bool IsDead() const   { return m_Lifecycle.IsDead(); }
    int  GetLives() const { return m_Lifecycle.Lives(); }
    bool IsStunned() const{ return m_Lifecycle.IsStunned(); }

    void Kill();
    void DebugKill();  // debug 主控台用：無視無敵/暈眩/彈跳，直接致死

    void Respawn();

    void AddBombCount() { m_CurrentBombs++; }
    // 死亡→Respawn 會把 m_CurrentBombs 歸 0；若場上仍有玩家放的炸彈，事後爆炸時
    // BombManager 會再呼叫 DecBombCount，沒下限保護會讓計數變負數，導致玩家事實上
    // 可以放比 m_MaxBombs 多 1 顆。Clamp 至 0 修掉這個對戰時可重現的 bug。
    void DecBombCount() { if (m_CurrentBombs > 0) m_CurrentBombs--; }
    bool CanPlaceBomb() const { return m_CurrentBombs < EffectiveMaxBombs(); }

	Direction GetDirection() const { return m_CurrentDir; }  // 取得當前方向

    // 通知玩家「他放在 (gx, gy) 的炸彈正在那一格」— 玩家會把該格暫時加入忽略碰撞清單，
    // 走離後 (IgnoreBombClearance) 才把該格從清單移除。BombManager 在 PlaceBomb 後呼叫。
    void OnPlacedBombAt(int gx, int gy) {
        for (const auto& b : m_IgnoreBombs)  // 去重，避免每幀重複加入同一格
            if (b.first == gx && b.second == gy) return;
        m_IgnoreBombs.push_back({ gx, gy });
    }

    bool HasKey() const { return m_HasKey; }
    void SetKey(bool key) { m_HasKey = key; }

    // 死亡時若持有鑰匙，會在 Kill() 內標記為「待掉落」並清掉持有狀態。
    // GameSession 每幀詢問一次：回 true 表示該把鑰匙實體放回世界 (只觸發一次)。
    // 「死亡掉鑰匙」這條遊戲規則因此留在 Player，不再洩漏進 GameSession::Update。
    bool ConsumeDroppedKey() {
        if (!m_DroppedKeyPending) return false;
        m_DroppedKeyPending = false;
        return true;
    }

	Team GetTeam() const { return m_Team; }

    int GetPlayerID() const { return m_PlayerID; };

    void ActivateSpeedBoost() { m_SpeedBoostTimer = Constants::Player::kSpeedBoostFrames; }
    void CancelSpeedBoost()   { m_SpeedBoostTimer = -1; }  // 立即解除加速 (作弊關閉時用)
    void IncreaseMaxBombs();
    // 作弊開啟時回報上限值；不影響玩家真實火力 (m_Firepower)。
    int GetFirepower() const { return m_CheatStats ? Constants::Player::kFirepowerCap : m_Firepower; }
    void IncreaseFirepower();

    // ------- 作弊模式 (CheatManager 透過這些 public API 操作，不碰私有狀態) -------
    void SetGodMode(bool on) { m_Lifecycle.SetGodMode(on); }
    bool IsGodMode() const   { return m_Lifecycle.IsGodMode(); }
    // 炸彈數 / 火力以「覆寫」方式拉滿：開啟期間有效數值=上限，但不動玩家真實進度
    // (m_MaxBombs / m_Firepower)。關閉的瞬間即自動還原為正常數值 — 連作弊期間
    // 正常吃到的道具一併保留，不需另外存檔/還原。
    void SetCheatStats(bool on) { m_CheatStats = on; }

    InputController* GetController() const { return m_Controller.get(); }

    // 靜止時是否自動歸位到格中心：僅 AI 開啟 (避免 bot 停在格角被波及)；
    // 人類玩家關閉，否則停下會被「硬拽」到格中心、手感差又易被預判。
    void SetAutoCenterIdle(bool on) { m_AutoCenterIdle = on; }

    // 移動速度倍率 (1.0 = 正常)。AI 進攻方設略小於 1，讓人類防守方能拉開身位 / 搶先抵達。
    void SetSpeedFactor(float f) { m_SpeedFactor = f; }

    bool TriggerBounce(Direction dir, int distance);  // 彈跳板

private:
    int m_GridX;
    int m_GridY;

    glm::vec2 m_Pos;  // 角色像素座標

    Direction m_CurrentDir;

    PlayerAnimator m_Animator;  // 方向→貼圖管理 (抽離自 Player，SRP/OCP)

    void ChangeDirection(Direction dir);

    // 暈眩視覺：依 PlayerLifecycle::TickStatus 套用旋轉 / 下沉 / 閃爍
    void ApplyStunVisuals(const PlayerLifecycle::TickStatus& s);

    // 確認是否可以移動
    bool IsColliding(float nextX, float nextY, const IWorldContext& worldContext, bool ignoreBombs = false);

    // 作弊開啟時回報上限值；否則回報玩家真實上限。
    int EffectiveMaxBombs() const { return m_CheatStats ? Constants::Player::kMaxBombsCap : m_MaxBombs; }

    int m_MaxBombs = Constants::Player::kInitialMaxBombs;        // 最大炸彈放置數量
    int m_CurrentBombs = 0;                                       // 當前場上炸彈數量
    bool m_CheatStats = false;  // 作弊：以覆寫方式拉滿炸彈數/火力 (不動真實進度，關閉即還原)

	// 暫時忽略的炸彈座標
    std::vector<std::pair<int, int>> m_IgnoreBombs;

    PlayerLifecycle m_Lifecycle;  // 死亡/暈眩/無敵/重生計時的狀態機 (抽離，SRP)

    bool m_HasKey = false;  // 角色是否有鑰匙
    bool m_DroppedKeyPending = false;  // 死亡時持有鑰匙 → 等 GameSession 取走並放回世界

    Team m_Team;
    bool m_AutoCenterIdle = false;  // true = 靜止時自動歸位格中心 (僅 AI)
    float m_SpeedFactor = 1.0f;     // 移動速度倍率 (AI 進攻方 <1，給防守方機動優勢)
    std::unique_ptr<InputController> m_Controller;

    int m_SpawnX;
    int m_SpawnY;

    int m_PlayerID;

	int m_SpeedBoostTimer = -1;  // 速度提升計時器

	int m_Firepower = Constants::Player::kInitialFirepower;  // 炸彈火力

    PlayerBounce m_Bounce;  // 彈跳板狀態機 (抽離自 Player，SRP)
};

#endif
