#include "Core/LevelSpawner.hpp"

#include <algorithm>
#include <random>

#include "Bot/BotProfile.hpp"
#include "Controller/BotController.hpp"
#include "Controller/HumanController.hpp"
#include "GameConstants.hpp"
#include "GameTypes.hpp"
#include "KeyBindings.hpp"
#include "LevelManager.hpp"
#include "Util/Keycode.hpp"

void LevelSpawner::SpawnPlayers(const LevelManager& levelManager,
                                const MatchConfig& config,
                                const KeyBindings* keys,
                                Util::Renderer& root,
                                std::vector<std::shared_ptr<Player>>& outPlayers,
                                Player*& outDefender,
                                Player*& outHumanAttacker) const {
    outDefender = nullptr;
    outHumanAttacker = nullptr;

    // 出生點 (進攻方隨機洗牌避免固定面對同一名 AI)
    auto defSpawn = levelManager.GetDefenderSpawn();
    auto atkSpawns = levelManager.GetAttackerSpawns();

    static std::random_device rd;
    static std::mt19937 g(rd());
    std::shuffle(atkSpawns.begin(), atkSpawns.end(), g);

    // 玩家 1 永遠是守方 (人類)；作弊 / HUD 命數都以玩家 1 為對象。
    const Control p1Ctrl = keys ? keys->p[0]
        : Control{ Util::Keycode::W, Util::Keycode::S, Util::Keycode::A, Util::Keycode::D, Util::Keycode::SPACE, Util::Keycode::E };
    auto ctrl1P = std::make_unique<HumanController>(p1Ctrl);

    auto defender = std::make_shared<Player>(defSpawn.first, defSpawn.second, Team::DEFENDER, std::move(ctrl1P), 0);
    outPlayers.push_back(defender);
    root.AddChild(defender);
    outDefender = defender.get();

    // 進攻方依「選擇隊伍」的席位設定生成 (slot 0 = 玩家 2，1..7 = 電腦)，依序填入地圖出生點。
    // 玩家 2 為人類時用方向鍵 + 右 Shift 放炸彈 (ENTER 留給暫停)。不會有 AI 防守。
    int nextPlayerId = 1;
    int spawnIdx = 0;
    for (int slot = 0; slot < MatchConfig::kMaxAttackers && spawnIdx < static_cast<int>(atkSpawns.size()); slot++) {
        const MatchConfig::SlotMode mode = config.AttackerSlot(slot);
        if (mode == MatchConfig::SlotMode::Off) continue;

        std::unique_ptr<InputController> ctrl;
        std::shared_ptr<const IBotProfile> botProfile;  // 非人類席位才有；用來決定移動速度
        if (mode == MatchConfig::SlotMode::Human) {
            const Control p2Ctrl = keys ? keys->p[1]
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
        if (mode == MatchConfig::SlotMode::Human) outHumanAttacker = attacker.get();   // 玩家2 (作弊對象)
        else {
            attacker->SetAutoCenterIdle(true);
            // 移動速度由性格決定 (狂戰快 0.95、謹慎慢 0.78) + 依 id 的小幅抖動：不同性格、
            // 同性格不同個體都看得出差異，皆 < 防守方 1.0；不再整排同步、同速移動 (「AI 太一致」)。
            attacker->SetSpeedFactor(botProfile->MoveSpeedScale() + (attacker->GetPlayerID() % 3) * 0.02f);
        }
        outPlayers.push_back(attacker);
        root.AddChild(attacker);
    }

    // 安全網：至少要有 1 名進攻方 (設定全關時，補一個 AI)
    if (spawnIdx == 0 && !atkSpawns.empty()) {
        const auto& spawn = atkSpawns[0];
        auto profile = BotProfileFactory::Default();
        auto attacker = std::make_shared<Player>(spawn.first, spawn.second, Team::ATTACKER,
                                                 std::make_unique<BotController>(profile, 0), nextPlayerId++);
        attacker->SetAutoCenterIdle(true);
        attacker->SetSpeedFactor(profile->MoveSpeedScale());  // AI：依性格決定速度
        outPlayers.push_back(attacker);
        root.AddChild(attacker);
    }
}

void LevelSpawner::SpawnSpirits(const LevelManager& levelManager,
                                const MatchConfig& config,
                                Util::Renderer& root,
                                std::vector<std::shared_ptr<Spirit>>& outSpirits) const {
    if (!config.SpiritsEnabled()) return;
    for (const auto& sp : levelManager.GetSpiritSpawns()) {
        auto spirit = std::make_shared<Spirit>(sp.first, sp.second);
        outSpirits.push_back(spirit);
        root.AddChild(spirit);
    }
}

void LevelSpawner::SpawnTurrets(const LevelManager& levelManager,
                                const MatchConfig& config,
                                Util::Renderer& root,
                                TurretManager& turretManager) const {
    if (!config.TurretsEnabled()) return;
    for (const auto& sp : levelManager.GetTurretSpawns()) {
        turretManager.AddTurret(std::make_shared<RotatingTurret>(sp.first, sp.second, Direction::DOWN), root);
    }
}
