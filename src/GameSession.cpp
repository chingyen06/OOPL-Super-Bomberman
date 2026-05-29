#include "GameSession.hpp"

#include <algorithm>
#include <chrono>
#include <random>
#include <string>

#include "Controller/BotController.hpp"
#include "Controller/HumanController.hpp"
#include "GameConstants.hpp"
#include "GameWorldContext.hpp"
#include "Util/Keycode.hpp"
#include "Util/Logger.hpp"

GameSession::GameSession(Util::Renderer& root) : m_Root(root) {}

void GameSession::LoadLevel(int levelIndex) {
    LOG_INFO("Loading Level " + std::to_string(levelIndex) + "...");

    // 清理上一局可能殘留的實體
    Clear();

    const std::string levelPath = RESOURCE_DIR"/Map/level_" + std::to_string(levelIndex) + ".txt";

    m_LevelManager.LoadLevel(levelPath, m_InteractableManager, m_Root);
    m_LevelManager.AttachToRoot(m_Root);

    const int totalChests = m_InteractableManager.GetObjectiveCount();
    m_UIManager.Init(m_Root, totalChests);

    // 出生點
    auto defSpawn = m_LevelManager.GetDefenderSpawn();
    auto atkSpawns = m_LevelManager.GetAttackerSpawns();

    static std::random_device rd;
    static std::mt19937 g(rd());
    std::shuffle(atkSpawns.begin(), atkSpawns.end(), g);

    auto ctrl1P = std::make_unique<HumanController>(Control{
        Util::Keycode::W,
        Util::Keycode::S,
        Util::Keycode::A,
        Util::Keycode::D,
        Util::Keycode::SPACE
    });

    auto defender = std::make_shared<Player>(defSpawn.first, defSpawn.second, Team::DEFENDER, std::move(ctrl1P), 0);
    m_Players.push_back(defender);
    m_Root.AddChild(defender);

    // 依地圖標示的 attacker spawn 數量產生對應數量的 AI
    // BotController 用 playerID 做 phase offset，避免所有 AI 第一幀同時決策
    int nextPlayerId = 1;
    for (const auto& spawn : atkSpawns) {
        const int phase = nextPlayerId % Constants::Bot::kReactionFrames;
        auto attacker = std::make_shared<Player>(spawn.first, spawn.second, Team::ATTACKER,
                                                  std::make_unique<BotController>(phase), nextPlayerId++);
        m_Players.push_back(attacker);
        m_Root.AddChild(attacker);
    }

    for (const auto& sp : m_LevelManager.GetSpiritSpawns()) {
        auto spirit = std::make_shared<Spirit>(sp.first, sp.second);
        m_Spirits.push_back(spirit);
        m_Root.AddChild(spirit);
    }

    for (const auto& sp : m_LevelManager.GetTurretSpawns()) {
        m_TurretManager.AddTurret(std::make_shared<RotatingTurret>(sp.first, sp.second, Direction::DOWN), m_Root);
    }

    m_GameTime = Constants::Game::kRoundDurationFrames;
}

void GameSession::Update() {
    if (m_GameTime > 0)
        m_GameTime--;

    if (m_GameTime % Constants::Game::kFPS == 0) {
        const int seconds = m_GameTime / Constants::Game::kFPS;
        LOG_INFO("Time Remaining: " + std::to_string(seconds) + "s");
    }

    m_BombManager.Update(m_LevelManager, m_InteractableManager, m_Root, m_Players);
    m_InteractableManager.Update(m_Players, m_Root);

    auto statusList = m_InteractableManager.GetObjectiveStatusList();
    m_UIManager.Update(m_GameTime, m_Players, statusList, m_Root);

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

        // 「死亡掉鑰匙」的判斷在 Player::Kill() 內；這裡只負責把鑰匙實體放回世界
        if (player->ConsumeDroppedKey()) {
            LOG_INFO("Player dropped the Key!");
            m_InteractableManager.AddKey(player->GetGridX(), player->GetGridY(), m_Root);
        }
    }

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
}

void GameSession::Clear() {
    m_InteractableManager.Clear(m_Root);
    m_BombManager.Clear(m_Root);
    m_LevelManager.Clear(m_Root);
    m_UIManager.Clear(m_Root);
    m_TurretManager.Clear(m_Root);

    for (auto& s : m_Spirits) m_Root.RemoveChild(s);
    m_Spirits.clear();

    for (auto& player : m_Players) m_Root.RemoveChild(player);
    m_Players.clear();
}

bool GameSession::IsAttackerWin() const {
    return m_InteractableManager.AreAllObjectivesComplete();
}
