#ifndef BOT_NAVIGATOR_HPP
#define BOT_NAVIGATOR_HPP

#include <vector>
#include <memory>

class LevelManager;
class BombManager;
class DangerMap;
class Spirit;
class TurretManager;
class Player;

// 封裝「某隻 bot 在某一幀如何看待這張地圖」：障礙判定、各策略用的 A* 成本評估、
// 炸彈火力射線與視線判斷。原本這些是 AIManager::Update 內散落的 lambda + 5 份幾乎
// 重複的 cost function；收斂成一個有明確介面、private 狀態的協作者類別 (而非 file-local
// 的 data struct)。由 AIManager 每幀為每隻 bot 建立一個，只持有 reference，生命週期不超過該幀。
class BotNavigator {
public:
    BotNavigator(const LevelManager& lm, const BombManager& bm, const DangerMap& danger,
                 const std::vector<std::shared_ptr<Spirit>>& spirits, const TurretManager& turrets,
                 const std::vector<std::shared_ptr<Player>>& players, const Player* self, int firepower);

    // 障礙查詢
    bool IsSpiritAt(int x, int y) const;     // 源石精靈所在格 (攻擊方碰到即死，hard obstacle)
    bool IsTurretAt(int x, int y) const;     // 砲台所在格 (撞上去會卡住)
    bool IsBlockedByOther(int x, int y) const; // 其他活著的玩家 (soft obstacle)

    // A* 成本函式：回傳 -1 表示不可走，正數為步進成本
    int SafeWalkCost(int x, int y) const;    // 一般安全行走 (避開危險/炸彈/精靈/砲台)
    int RushCost(int x, int y) const;        // 衝目標用：避開正在燒的火焰，但「即將爆炸尚未噴火」的格可走 (高成本)
    int RetreatCost(int x, int y) const;     // 逃離危險用 (同上但不避開致命格)
    int BrickCost(int x, int y) const;       // 炸牆開路 (磚塊可走但成本高)
    int SuicideCost(int x, int y) const;     // 自殺攻擊 (無視火焰，只有牆與砲台擋)

    // 炸彈火力 / 視線判斷
    bool BombReaches(int bx, int by, int tx, int ty) const;   // (bx,by) 放彈火焰能否掃到 (tx,ty)
    bool BombHitsAnySpirit(int bx, int by) const;             // 放彈能否炸到任一精靈
    bool HasLineOfSight(int bx, int by, int tx, int ty) const; // 同行/列且中間無牆

private:
    static constexpr int kOtherPlayerPenalty = 25;  // 走過其他玩家格的高成本，強烈傾向繞路

    const LevelManager& m_Lm;
    const BombManager& m_Bm;
    const DangerMap& m_Danger;
    const std::vector<std::shared_ptr<Spirit>>& m_Spirits;
    const TurretManager& m_Turrets;
    const std::vector<std::shared_ptr<Player>>& m_Players;
    const Player* m_Self;
    int m_Fp;
};

#endif
