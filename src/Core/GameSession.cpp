#include "GameSession.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <random>
#include <string>

#include <imgui.h>

#include "Controller/BotController.hpp"
#include "Controller/HumanController.hpp"
#include "GameConstants.hpp"
#include "GameWorldContext.hpp"
#include "GridCoord.hpp"
#include "KeyBindings.hpp"
#include "SaveData.hpp"
#include "Weapons/WeaponFactory.hpp"
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
        if (mode == MatchConfig::SlotMode::Human) {
            const Control p2Ctrl = m_Keys ? m_Keys->p[1]
                : Control{ Util::Keycode::UP, Util::Keycode::DOWN, Util::Keycode::LEFT, Util::Keycode::RIGHT, Util::Keycode::RSHIFT, static_cast<Util::Keycode>(0) };
            ctrl = std::make_unique<HumanController>(p2Ctrl);
        }
        else {
            // 依席位給不同性格，讓同場每隻 AI 想法各異 (獵人 / 拾荒 / 狂戰 / 謹慎輪替)。
            ctrl = std::make_unique<BotController>(BotProfileFactory::ForSlot(slot),
                                                   nextPlayerId % Constants::Bot::kReactionFrames);
        }
        const auto& spawn = atkSpawns[spawnIdx++];
        auto attacker = std::make_shared<Player>(spawn.first, spawn.second, Team::ATTACKER, std::move(ctrl), nextPlayerId++);
        if (mode == MatchConfig::SlotMode::Human) m_HumanPlayer2 = attacker.get();   // 玩家2 (作弊對象)
        else attacker->SetAutoCenterIdle(true);                                       // AI 才自動歸位格中心
        m_Players.push_back(attacker);
        m_Root.AddChild(attacker);
    }

    // 安全網：至少要有 1 名進攻方 (設定全關時，補一個 AI)
    if (spawnIdx == 0 && !atkSpawns.empty()) {
        const auto& spawn = atkSpawns[0];
        auto attacker = std::make_shared<Player>(spawn.first, spawn.second, Team::ATTACKER,
                                                 std::make_unique<BotController>(BotProfileFactory::Default(), 0), nextPlayerId++);
        attacker->SetAutoCenterIdle(true);  // AI
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
    m_Weapon = WeaponFactory::Create(m_Config.DefenderWeapon());
    m_Charge = 0.0f;
    m_DefenderKills = 0;
    m_ChargeBg   = std::make_shared<UIImage>(RESOURCE_DIR"/Image/charge_bg.png",   -1000.0f, -1000.0f, 86.0f);
    m_ChargeFill = std::make_shared<UIImage>(RESOURCE_DIR"/Image/charge_fill.png", -1000.0f, -1000.0f, 87.0f);
    m_Root.AddChild(m_ChargeBg);
    m_Root.AddChild(m_ChargeFill);

    m_GameTime = m_Config.RoundSeconds() * Constants::Game::kFPS;
}

void GameSession::AddEffect(float px, float py, const std::string& sprite, int frames) {
    auto e = std::make_shared<UIImage>(sprite, px, py, 85.0f);
    m_Root.AddChild(e);
    m_WeaponFx.emplace_back(e, frames);
}

void GameSession::UpdateDefenderWeapon() {
    constexpr float kPerFrame  = 1.0f / (12.0f * Constants::Game::kFPS);  // 約 12 秒充滿
    constexpr float kKillBonus = 0.34f;                                   // 每擊倒 1 名大幅回充

    // 作弊 P1 (防守方)：充能永遠維持滿格，武器隨時可發動。
    const bool cheatFullCharge = m_CheatManager.IsEnabled(0);
    if (cheatFullCharge) m_Charge = 1.0f;

    // 時間慢回 (作弊時已滿，不需再回)
    if (!cheatFullCharge && m_HumanPlayer && !m_HumanPlayer->IsDead() && m_Charge < 1.0f) {
        m_Charge += kPerFrame;
        if (m_Charge > 1.0f) m_Charge = 1.0f;
    }
    // 滿了且按發動鍵 → 發動 (暈倒中不能發動武器)，依擊倒回充
    if (m_Weapon && m_HumanPlayer && !m_HumanPlayer->IsDead() && !m_HumanPlayer->IsStunned() && m_Charge >= 1.0f) {
        InputController* ctrl = m_HumanPlayer->GetController();
        if (ctrl && ctrl->IsWeaponJustPressed()) {
            const int kills = m_Weapon->Fire(*m_HumanPlayer, m_Players, m_LevelManager, m_Root, *this);
            m_DefenderKills += kills;
            float bonus = kills * kKillBonus;
            m_Charge = (bonus > 1.0f) ? 1.0f : bonus;  // 歸零後依擊倒回充
            if (cheatFullCharge) m_Charge = 1.0f;      // 作弊：發動後立即補滿
        }
    }
    // 特效壽命
    for (auto it = m_WeaponFx.begin(); it != m_WeaponFx.end();) {
        if (--it->second <= 0) { m_Root.RemoveChild(it->first); it = m_WeaponFx.erase(it); }
        else ++it;
    }
    // 屏障到期還原地形
    m_LevelManager.TickTemporary(m_Root);

    // 充能條：浮在防守方頭上，由左往右填滿
    if (m_ChargeBg && m_ChargeFill) {
        if (m_HumanPlayer && !m_HumanPlayer->IsDead()) {
            const glm::vec2 p = m_HumanPlayer->GetPixelPos();
            const float full = 32.0f, by = p.y + 62.0f, left = p.x - full * 0.5f;
            const float c = m_Charge < 0.02f ? 0.02f : m_Charge;
            m_ChargeBg->SetPosition(p.x, by);
            m_ChargeFill->SetScale(c, 1.0f);
            m_ChargeFill->SetPosition(left + full * c * 0.5f, by);
        }
        else {
            m_ChargeBg->SetPosition(-1000.0f, -1000.0f);
            m_ChargeFill->SetPosition(-1000.0f, -1000.0f);
        }
    }
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

    if (m_GameTime > 0)
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

        // 「死亡掉鑰匙」的判斷在 Player::Kill() 內；這裡只負責把鑰匙實體放回世界。
        // 掉落點需避開炸彈格 (會砸倒人) 與既有鑰匙/物件 (避免兩把重疊)。
        if (player->ConsumeDroppedKey()) {
            const auto drop = FindKeyDropTile(player->GetGridX(), player->GetGridY());
            LOG_INFO("Player dropped the Key!");
            m_InteractableManager.AddKey(drop.first, drop.second, m_Root);
        }
    }

    // HUD (計時 / 皇冠 / 鑰匙指示) 必須在玩家移動「之後」才更新，否則皇冠等浮標會
    // 沿用上一幀的座標而落後玩家一格 (走路時皇冠拖在後面)。
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
        if (m_ShowDanger) {
            for (int y = 0; y < GridCoord::kMapHeight; ++y)
                for (int x = 0; x < GridCoord::kMapWidth; ++x)
                    if (m_AIManager.IsDangerAt(x, y))
                        dangerCells.emplace_back(GridCoord::ToPixelX(x), GridCoord::ToPixelY(y));
        }
        RenderDebugConsole();  // 文字資訊與控制鈕改由 ImGui 主控台呈現
    }
    // 危險地圖紅塊仍由場景空間的 overlay 繪製 (ImGui 無法畫在遊戲世界裡)
    m_DebugOverlay.Update(m_Root, {}, dangerCells);

    UpdateDefenderWeapon();  // 防守方充能 / 發動 / 特效 / 充能條
}

void GameSession::RenderDebugConsole() {
    ImGui::SetNextWindowPos(ImVec2(12, 12), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(320, 0), ImGuiCond_FirstUseEver);
    ImGui::Begin("Game Debug Console (F3)");

    if (ImGui::CollapsingHeader("FPS Panel", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("FPS: %.1f", m_Fps);
    }

    if (ImGui::CollapsingHeader("Match", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Level: %d", m_CurrentLevelIndex);
        ImGui::Text("Time left: %d s", (m_GameTime > 0 ? m_GameTime : 0) / Constants::Game::kFPS);
        int opened = 0;
        for (bool done : m_InteractableManager.GetObjectiveStatusList()) if (done) opened++;
        ImGui::Text("Chests opened: %d / %d", opened, m_InteractableManager.GetObjectiveCount());
        if (ImGui::Button("+30s")) m_GameTime += 30 * Constants::Game::kFPS;
        ImGui::SameLine();
        if (ImGui::Button("-30s")) { m_GameTime -= 30 * Constants::Game::kFPS; if (m_GameTime < 0) m_GameTime = 0; }
        ImGui::SameLine();
        if (ImGui::Button("End now (Defender Win)")) m_GameTime = 0;
    }

    if (m_Profile && ImGui::CollapsingHeader("Player Wallet", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Coins: %d", m_Profile->Coins());
        if (ImGui::Button("+100"))  m_Profile->AddCoins(100);  ImGui::SameLine();
        if (ImGui::Button("+500"))  m_Profile->AddCoins(500);  ImGui::SameLine();
        if (ImGui::Button("+1000")) m_Profile->AddCoins(1000);
        if (ImGui::Button("Reset (0)")) m_Profile->SetCoins(0);
        ImGui::SameLine();
        static int amount = 50;
        ImGui::SetNextItemWidth(90);
        ImGui::InputInt("##setcoins", &amount);
        ImGui::SameLine();
        if (ImGui::Button("Set")) m_Profile->SetCoins(amount);
    }

    if (ImGui::CollapsingHeader("Players", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Players: %d   Spirits: %d",
                    static_cast<int>(m_Players.size()), static_cast<int>(m_Spirits.size()));
        auto playerLine = [](const char* tag, Player* p) {
            ImGui::Text("%s: grid(%d,%d)  lives %d  fp %d  %s", tag,
                        p->GetGridX(), p->GetGridY(), p->GetLives(), p->GetFirepower(),
                        p->IsDead() ? "DEAD" : (p->IsStunned() ? "STUN" : "OK"));
        };
        if (m_HumanPlayer)  playerLine("P1", m_HumanPlayer);
        if (m_HumanPlayer2) playerLine("P2", m_HumanPlayer2);
        else                ImGui::TextDisabled("P2: (no human P2)");

        bool c1 = m_CheatManager.IsEnabled(0);
        if (ImGui::Checkbox("Cheat P1 (god / max)", &c1)) m_CheatManager.Toggle(0);
        if (m_HumanPlayer2) {
            bool c2 = m_CheatManager.IsEnabled(1);
            if (ImGui::Checkbox("Cheat P2 (god / max)", &c2)) m_CheatManager.Toggle(1);
        }
        else {
            ImGui::TextDisabled("Cheat P2 (no human P2)");
        }
        // Kill 無視無敵時間 (DebugKill)
        if (ImGui::Button("Kill P1") && m_HumanPlayer) m_HumanPlayer->DebugKill();
        if (m_HumanPlayer2) { ImGui::SameLine(); if (ImGui::Button("Kill P2")) m_HumanPlayer2->DebugKill(); }
        if (ImGui::Button("Kill all attackers")) {
            for (auto& p : m_Players)
                if (p->GetTeam() == Team::ATTACKER && !p->IsDead()) p->DebugKill();
        }
        if (ImGui::Button("Clear spirits")) {
            for (auto& s : m_Spirits) m_Root.RemoveChild(s);
            m_Spirits.clear();
        }
    }

    if (ImGui::CollapsingHeader("Visualize", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Show danger map (red tiles)", &m_ShowDanger);
    }

    ImGui::End();
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

    // 武器 / 充能 / 特效
    m_Weapon.reset();
    if (m_ChargeBg)   { m_Root.RemoveChild(m_ChargeBg);   m_ChargeBg.reset(); }
    if (m_ChargeFill) { m_Root.RemoveChild(m_ChargeFill); m_ChargeFill.reset(); }
    for (auto& e : m_WeaponFx) m_Root.RemoveChild(e.first);
    m_WeaponFx.clear();
    m_Charge = 0.0f;
    m_DefenderKills = 0;
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

    r.defenderKills = m_DefenderKills;

    // 金幣獎勵：底分 150 + 防守方獲勝 350 + 每守住寶箱 50 + 每武器擊倒進攻方 100。
    // 永遠為正值，讓結算畫面一定看得到「+」。
    int coins = 150;
    if (defenderWin) coins += 350;
    coins += r.chestsDefended * 50;
    coins += r.defenderKills * 100;  // 擊殺加成計入金幣
    r.coinsEarned = coins;

    return r;
}
