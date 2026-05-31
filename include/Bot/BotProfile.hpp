#ifndef BOT_PROFILE_HPP
#define BOT_PROFILE_HPP

#include <memory>

// 一隻 AI 的「性格 / 想法」。把原本寫死、所有 bot 共用的決策參數抽成一個策略物件
// (Strategy Pattern)，由 BotController 持有、AIManager 查詢。
//   - OCP：新增一種性格 = 新增一個子類別，AIManager 完全不用改。
//   - DIP：AIManager 只依賴這個抽象介面，不認識任何具體性格。
// 不同 bot 注入不同性格，於是同場的 AI 想法各異，而非全部一個樣。
class IBotProfile {
public:
    virtual ~IBotProfile() = default;
    virtual const char* Name() const = 0;

    // 兩次決策之間最少幀數：越小反應越快、越少在原地發呆。
    virtual int ReactionFrames() const = 0;

    // 目標取向：true = 優先追殺防守方；false = 優先撿鑰匙 / 道具 / 開寶箱。
    virtual bool HuntsDefender() const = 0;

    // 主動朝防守方追擊 / 放炸彈的最大曼哈頓距離。越大越敢遠距離開炸驅趕。
    virtual int BombChaseRange() const = 0;

    // 自殺式突擊的容許距離 (加在基礎門檻之上)。越大越敢拼命。
    virtual int SuicideBoldness() const = 0;
};

// ---- 具體性格 (header-only inline，與 TileSet / BotController 同風格) ----

// 獵人：積極追殺防守方、敢開炸、反應快。
class HunterBotProfile : public IBotProfile {
public:
    const char* Name() const override { return "Hunter"; }
    int  ReactionFrames()  const override { return 4; }
    bool HuntsDefender()   const override { return true; }
    int  BombChaseRange()  const override { return 4; }
    int  SuicideBoldness() const override { return 2; }
};

// 拾荒者：優先衝鑰匙 / 道具 / 寶箱，少正面衝突。
class CollectorBotProfile : public IBotProfile {
public:
    const char* Name() const override { return "Collector"; }
    int  ReactionFrames()  const override { return 6; }
    bool HuntsDefender()   const override { return false; }
    int  BombChaseRange()  const override { return 2; }
    int  SuicideBoldness() const override { return 0; }
};

// 狂戰士：反應極快、膽量極大，會搶在爆炸前行動、不惜冒險。
class BerserkerBotProfile : public IBotProfile {
public:
    const char* Name() const override { return "Berserker"; }
    int  ReactionFrames()  const override { return 3; }
    bool HuntsDefender()   const override { return true; }
    int  BombChaseRange()  const override { return 5; }
    int  SuicideBoldness() const override { return 4; }
};

// 謹慎者：反應慢、不冒險，會等危險完全過去才動 (刻意保留的「穩健」性格)。
class CautiousBotProfile : public IBotProfile {
public:
    const char* Name() const override { return "Cautious"; }
    int  ReactionFrames()  const override { return 10; }
    bool HuntsDefender()   const override { return false; }
    int  BombChaseRange()  const override { return 2; }
    int  SuicideBoldness() const override { return 0; }
};

// 性格工廠：依席位輪流配置，讓同場每隻 AI 想法不同。性格無狀態，可安全共用單例。
class BotProfileFactory {
public:
    static std::shared_ptr<const IBotProfile> ForSlot(int slotIndex) {
        switch (((slotIndex % 4) + 4) % 4) {
            case 0:  return Hunter();
            case 1:  return Collector();
            case 2:  return Berserker();
            default: return Cautious();
        }
    }
    static std::shared_ptr<const IBotProfile> Default() { return Hunter(); }

private:
    static std::shared_ptr<const IBotProfile> Hunter() {
        static std::shared_ptr<const IBotProfile> p = std::make_shared<const HunterBotProfile>();
        return p;
    }
    static std::shared_ptr<const IBotProfile> Collector() {
        static std::shared_ptr<const IBotProfile> p = std::make_shared<const CollectorBotProfile>();
        return p;
    }
    static std::shared_ptr<const IBotProfile> Berserker() {
        static std::shared_ptr<const IBotProfile> p = std::make_shared<const BerserkerBotProfile>();
        return p;
    }
    static std::shared_ptr<const IBotProfile> Cautious() {
        static std::shared_ptr<const IBotProfile> p = std::make_shared<const CautiousBotProfile>();
        return p;
    }
};

#endif
