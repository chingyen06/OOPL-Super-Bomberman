#ifndef LEVEL_SPAWNER_HPP
#define LEVEL_SPAWNER_HPP

#include <memory>
#include <utility>
#include <vector>

#include "MatchConfig.hpp"
#include "Player.hpp"
#include "Spirit.hpp"
#include "Turret/TurretManager.hpp"
#include "Util/Renderer.hpp"

class LevelManager;
class KeyBindings;

// 把「依本場設定生成玩家 / 源石 / 砲台」從 GameSession 抽出 (SRP)。
// 本類別只負責「依設定產出實體」，不持有它們、不每幀推進；GameSession::LoadLevel
// 呼叫一次後拿到結果，後續仍由 GameSession 持有 / 推進 / 清理。
//
// 也用方法切分取代過去在 GameSession::LoadLevel 內的 if(SpiritsEnabled) / if(TurretsEnabled) 分支：
// 新增可選實體型別 → 為 LevelSpawner 加一個 SpawnXxx + GameSession 呼叫一次即可，不必再
// 動 LoadLevel 主流程 (OCP 友善)。
class LevelSpawner {
public:
    // 生成 1 名守方 (玩家1，永遠是人類) + 依 config 生成進攻方 (玩家2 為人類 / 其餘為 AI)，
    // 並把它們加到 root。最少會有 1 名進攻方 (config 全 Off 時補一個 AI)。
    // outDefender / outHumanAttacker 為非擁有指標 (指向 outPlayers 內物件，給作弊/HUD 用)。
    void SpawnPlayers(const LevelManager& levelManager,
                      const MatchConfig& config,
                      const KeyBindings* keys,
                      Util::Renderer& root,
                      std::vector<std::shared_ptr<Player>>& outPlayers,
                      Player*& outDefender,
                      Player*& outHumanAttacker) const;

    // 依 config 決定要不要生 Spirit / Turret (各自 if 條件留在這裡，
    // GameSession::LoadLevel 不再混雜這些細節)。
    void SpawnSpirits(const LevelManager& levelManager,
                      const MatchConfig& config,
                      Util::Renderer& root,
                      std::vector<std::shared_ptr<Spirit>>& outSpirits) const;

    void SpawnTurrets(const LevelManager& levelManager,
                      const MatchConfig& config,
                      Util::Renderer& root,
                      TurretManager& turretManager) const;
};

#endif
