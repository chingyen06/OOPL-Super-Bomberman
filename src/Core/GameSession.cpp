#include "GameSession.hpp"

#include <chrono>
#include <cstdio>
#include <string>

#include "GameConstants.hpp"
#include "GameWorldContext.hpp"
#include "GridCoord.hpp"
#include "KeyBindings.hpp"
#include "SaveData.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"
#include "Util/Logger.hpp"

GameSession::GameSession(Util::Renderer& root) : m_Root(root) {}

// LoadLevel 只做高層協調：載地圖 / 委派 LevelSpawner 生實體 / 初始 UI 與武器充能。
// 玩家 / 源石 / 砲台的「依設定生成」細節皆已委派 (SRP)；新增可選實體型別只需在 LevelSpawner
// 加一個 Spawn 方法 + 這裡呼叫一行，不必動既有生成邏輯 (OCP 友善)。
void GameSession::LoadLevel(int levelIndex) {
    LOG_INFO("Loading Level " + std::to_string(levelIndex) + "...");

    m_CurrentLevelIndex = levelIndex;
    Clear();  // 清理上一局可能殘留的實體

    const std::string levelPath = RESOURCE_DIR"/Map/level_" + std::to_string(levelIndex) + ".txt";
    m_LevelManager.LoadLevel(levelPath, TileSet::ForLevel(levelIndex), m_InteractableManager, m_Root);
    m_LevelManager.AttachToRoot(m_Root);

    m_UIManager.Init(m_Root, m_InteractableManager.GetObjectiveCount());

    m_Spawner.SpawnPlayers(m_LevelManager, m_Config, m_Keys, m_Root,
                           m_Players, m_HumanPlayer, m_HumanPlayer2);
    m_Spawner.SpawnSpirits(m_LevelManager, m_Config, m_Root, m_Spirits);
    m_Spawner.SpawnTurrets(m_LevelManager, m_Config, m_Root, m_TurretManager);

    // 防守方武器 + 充能槽 (依本場選擇建立；充能條浮在防守方頭上)
    m_WeaponSystem.Init(m_Config.DefenderWeapon(), m_Root);

    m_GameTime = m_Config.RoundSeconds() * Constants::Game::kFPS;
}

void GameSession::Update() {
    // FPS 量測 (debug overlay 用) — 以實際更新間隔平滑估算
    const auto now = std::chrono::steady_clock::now();
    if (m_HasFrameTime) {
        const float dt = std::chrono::duration<float>(now - m_LastFrameTime).count();
        if (dt > 0.0f) m_Fps = m_Fps * 0.9f + (1.0f / dt) * 0.1f;
    }
    m_LastFrameTime = now;
    m_HasFrameTime = true;

    if (Util::Input::IsKeyUp(Util::Keycode::F3)) m_DebugOverlay.Toggle();

    if (m_GameTime > 0 && !m_DebugConsole.FreezeTimer())
        m_GameTime--;

    if (m_GameTime % Constants::Game::kFPS == 0) {
        const int seconds = m_GameTime / Constants::Game::kFPS;
        LOG_INFO("Time Remaining: " + std::to_string(seconds) + "s");
    }

    m_CheatManager.Update(m_HumanPlayer, m_HumanPlayer2);

    m_BombManager.Update(m_LevelManager, m_InteractableManager, m_TurretManager, m_Root, m_Players);
    m_InteractableManager.Update(m_Players, m_Root);

    m_AIManager.Update(m_Players, m_LevelManager, m_BombManager, m_InteractableManager, m_Spirits, m_TurretManager);
    m_TurretManager.Update(m_Players, m_LevelManager, m_BombManager, m_InteractableManager, m_Root);

    GameWorldContext worldContext(m_LevelManager, m_BombManager, m_InteractableManager, m_TurretManager);

    for (auto& player : m_Players) {
        player->Update(worldContext);

        if (!player->IsDead()) {
            if (player->GetController() && player->GetController()->IsPlaceBombJustPressed()) {
                m_BombManager.PlaceBomb(*player, m_LevelManager, m_InteractableManager, m_Root, m_Players);
            }
        }

        // 死亡掉鑰匙：把鑰匙實體放回世界 (避開炸彈格與既有物件)
        if (player->ConsumeDroppedKey()) {
            const auto drop = FindKeyDropTile(player->GetGridX(), player->GetGridY());
            LOG_INFO("Player dropped the Key!");
            m_InteractableManager.AddKey(drop.first, drop.second, m_Root);
        }
    }

    // HUD 須在玩家移動後才更新，否則皇冠等浮標會落後一格
    auto statusList = m_InteractableManager.GetObjectiveStatusList();
    m_UIManager.Update(m_GameTime, m_Players, statusList, m_Root,
                       m_CheatManager.IsEnabled(0) || m_CheatManager.IsEnabled(1));

    for (auto it = m_Spirits.begin(); it != m_Spirits.end();) {
        auto& spirit = *it;
        spirit->Update(m_Players, worldContext);

        if (spirit->ShouldDelete()) {
            m_Root.RemoveChild(spirit);
            it = m_Spirits.erase(it);
        }
        else {
            ++it;
        }
    }

    // ---- Debug 主控台 (F3) — 在 AIManager.Update 之後，危險地圖為最新 ----
    std::vector<std::pair<float, float>> dangerCells;
    if (m_DebugOverlay.IsEnabled()) {
        if (m_DebugConsole.ShowDanger()) {
            for (int y = 0; y < GridCoord::kMapHeight; ++y)
                for (int x = 0; x < GridCoord::kMapWidth; ++x)
                    if (m_AIManager.IsDangerAt(x, y))
                        dangerCells.emplace_back(GridCoord::ToPixelX(x), GridCoord::ToPixelY(y));
        }
        // 文字資訊與控制鈕由抽出的 DebugConsole 以 ImGui 呈現 (依賴明確傳入)
        m_DebugConsole.Render(m_CurrentLevelIndex, m_GameTime, m_Fps,
                              m_InteractableManager, m_Profile, m_Players, m_Spirits,
                              m_HumanPlayer, m_HumanPlayer2, m_CheatManager,
                              m_BombManager, m_TurretManager, m_WeaponSystem, m_Root);
    }
    // 危險地圖紅塊仍由場景空間的 overlay 繪製 (ImGui 無法畫在遊戲世界裡)
    m_DebugOverlay.Update(m_Root, {}, dangerCells);

    // 防守方充能 / 發動 / 特效 / 充能條 (作弊 P1 時維持滿格)
    m_WeaponSystem.Update(m_HumanPlayer, m_Players, m_LevelManager, m_CheatManager.IsEnabled(0));
}

void GameSession::Clear() {
    m_DebugOverlay.Clear(m_Root);
    m_InteractableManager.Clear(m_Root);
    m_BombManager.Clear(m_Root);
    m_LevelManager.Clear(m_Root);
    m_UIManager.Clear(m_Root);
    m_TurretManager.Clear(m_Root);

    for (auto& s : m_Spirits) m_Root.RemoveChild(s);
    m_Spirits.clear();

    for (auto& player : m_Players) m_Root.RemoveChild(player);
    m_Players.clear();
    m_HumanPlayer  = nullptr;  // 指向已銷毀的玩家，避免懸空
    m_HumanPlayer2 = nullptr;

    // 武器 / 充能 / 特效 (由子系統自行清理)
    m_WeaponSystem.Clear();
}

bool GameSession::IsAttackerWin() const {
    return m_InteractableManager.AreAllObjectivesComplete();
}

std::pair<int, int> GameSession::FindKeyDropTile(int sx, int sy) const {
    auto absI = [](int v) { return v < 0 ? -v : v; };
    auto valid = [&](int x, int y) {
        return GridCoord::InBounds(x, y)
            && m_LevelManager.IsWalkable(x, y)
            && !m_BombManager.IsBombAt(x, y)              // 避免和炸彈同格
            && !m_InteractableManager.IsBlocksBombAt(x, y);  // 避免和既有鑰匙/寶箱/道具重疊
    };
    if (valid(sx, sy)) return { sx, sy };
    // 由近到遠一圈圈找最近的合法格
    for (int r = 1; r < GridCoord::kMapWidth; ++r) {
        for (int dy = -r; dy <= r; ++dy) {
            for (int dx = -r; dx <= r; ++dx) {
                if (absI(dx) != r && absI(dy) != r) continue;  // 只看最外圈
                const int x = sx + dx, y = sy + dy;
                if (valid(x, y)) return { x, y };
            }
        }
    }
    return { sx, sy };  // 退路 (理論上不會走到)
}

MatchResult GameSession::BuildResult(bool defenderWin) const {
    MatchResult r;
    r.defenderWin  = defenderWin;
    r.totalChests  = m_InteractableManager.GetObjectiveCount();

    int opened = 0;
    for (bool done : m_InteractableManager.GetObjectiveStatusList()) if (done) opened++;
    r.chestsOpened   = opened;
    r.chestsDefended = r.totalChests - opened;

    const int remainingSeconds = (m_GameTime > 0 ? m_GameTime : 0) / Constants::Game::kFPS;
    r.elapsedSeconds = m_Config.RoundSeconds() - remainingSeconds;

    r.defenderKills = m_WeaponSystem.DefenderKills();

    // 金幣獎勵：底分 150 + 守方獲勝 350 + 每守住寶箱 50 + 每武器擊倒 100
    int coins = 150;
    if (defenderWin) coins += 350;
    coins += r.chestsDefended * 50;
    coins += r.defenderKills * 100;  // 擊殺加成計入金幣
    r.coinsEarned = coins;

    return r;
}
