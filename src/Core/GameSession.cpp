#include "GameSession.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <random>
#include <string>

#include "Controller/BotController.hpp"
#include "Controller/HumanController.hpp"
#include "GameConstants.hpp"
#include "GameWorldContext.hpp"
#include "GridCoord.hpp"
#include "KeyBindings.hpp"
#include "SaveData.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"
#include "Util/Logger.hpp"

GameSession::GameSession(Util::Renderer& root) : m_Root(root) {}

void GameSession::LoadLevel(int levelIndex) {
    LOG_INFO("Loading Level " + std::to_string(levelIndex) + "...");

    m_CurrentLevelIndex = levelIndex;

    // 清理上一局可能殘留的實體
    Clear();

    const std::string levelPath = RESOURCE_DIR"/Map/level_" + std::to_string(levelIndex) + ".txt";

    m_LevelManager.LoadLevel(levelPath, TileSet::ForLevel(levelIndex), m_InteractableManager, m_Root);
    m_LevelManager.AttachToRoot(m_Root);

    const int totalChests = m_InteractableManager.GetObjectiveCount();
    m_UIManager.Init(m_Root, totalChests);

    // 出生點
    auto defSpawn = m_LevelManager.GetDefenderSpawn();
    auto atkSpawns = m_LevelManager.GetAttackerSpawns();

    static std::random_device rd;
    static std::mt19937 g(rd());
    std::shuffle(atkSpawns.begin(), atkSpawns.end(), g);

    // 玩家 1 永遠是守方 (人類)；作弊 / HUD 命數都以玩家 1 為對象。
    const Control p1Ctrl = m_Keys ? m_Keys->p[0]
        : Control{ Util::Keycode::W, Util::Keycode::S, Util::Keycode::A, Util::Keycode::D, Util::Keycode::SPACE, Util::Keycode::E };
    auto ctrl1P = std::make_unique<HumanController>(p1Ctrl);

    auto defender = std::make_shared<Player>(defSpawn.first, defSpawn.second, Team::DEFENDER, std::move(ctrl1P), 0);
    m_Players.push_back(defender);
    m_Root.AddChild(defender);
    m_HumanPlayer = defender.get();

    // 進攻方依「選擇隊伍」的席位設定生成 (slot 0 = 玩家 2，1..7 = 電腦)，依序填入地圖出生點。
    // 玩家 2 為人類時用方向鍵 + 右 Shift 放炸彈 (ENTER 留給暫停)。不會有 AI 防守。
    int nextPlayerId = 1;
    int spawnIdx = 0;
    for (int slot = 0; slot < MatchConfig::kMaxAttackers && spawnIdx < static_cast<int>(atkSpawns.size()); slot++) {
        const MatchConfig::SlotMode mode = m_Config.AttackerSlot(slot);
        if (mode == MatchConfig::SlotMode::Off) continue;

        std::unique_ptr<InputController> ctrl;
        std::shared_ptr<const IBotProfile> botProfile;  // 非人類席位才有；用來決定移動速度
        if (mode == MatchConfig::SlotMode::Human) {
            const Control p2Ctrl = m_Keys ? m_Keys->p[1]
                : Control{ Util::Keycode::UP, Util::Keycode::DOWN, Util::Keycode::LEFT, Util::Keycode::RIGHT, Util::Keycode::RSHIFT, static_cast<Util::Keycode>(0) };
            ctrl = std::make_unique<HumanController>(p2Ctrl);
        }
        else {
            // 依席位給不同性格，讓同場每隻 AI 想法各異 (獵人 / 拾荒 / 狂戰 / 謹慎輪替)。
            botProfile = BotProfileFactory::ForSlot(slot);
            ctrl = std::make_unique<BotController>(botProfile,
                                                   nextPlayerId % Constants::Bot::kReactionFrames);
        }
        const auto& spawn = atkSpawns[spawnIdx++];
        auto attacker = std::make_shared<Player>(spawn.first, spawn.second, Team::ATTACKER, std::move(ctrl), nextPlayerId++);
        if (mode == MatchConfig::SlotMode::Human) m_HumanPlayer2 = attacker.get();   // 玩家2 (作弊對象)
        else {
            attacker->SetAutoCenterIdle(true);
            // 移動速度由性格決定 (狂戰快 0.95、謹慎慢 0.78) + 依 id 的小幅抖動：不同性格、
            // 同性格不同個體都看得出差異，皆 < 防守方 1.0；不再整排同步、同速移動 (「AI 太一致」)。
            attacker->SetSpeedFactor(botProfile->MoveSpeedScale() + (attacker->GetPlayerID() % 3) * 0.02f);
        }
        m_Players.push_back(attacker);
        m_Root.AddChild(attacker);
    }

    // 安全網：至少要有 1 名進攻方 (設定全關時，補一個 AI)
    if (spawnIdx == 0 && !atkSpawns.empty()) {
        const auto& spawn = atkSpawns[0];
        auto profile = BotProfileFactory::Default();
        auto attacker = std::make_shared<Player>(spawn.first, spawn.second, Team::ATTACKER,
                                                 std::make_unique<BotController>(profile, 0), nextPlayerId++);
        attacker->SetAutoCenterIdle(true);
        attacker->SetSpeedFactor(profile->MoveSpeedScale());  // AI：依性格決定速度
        m_Players.push_back(attacker);
        m_Root.AddChild(attacker);
    }

    if (m_Config.SpiritsEnabled()) {
        for (const auto& sp : m_LevelManager.GetSpiritSpawns()) {
            auto spirit = std::make_shared<Spirit>(sp.first, sp.second);
            m_Spirits.push_back(spirit);
            m_Root.AddChild(spirit);
        }
    }

    if (m_Config.TurretsEnabled()) {
        for (const auto& sp : m_LevelManager.GetTurretSpawns()) {
            m_TurretManager.AddTurret(std::make_shared<RotatingTurret>(sp.first, sp.second, Direction::DOWN), m_Root);
        }
    }

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
