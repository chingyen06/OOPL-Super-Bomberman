#ifndef IPLAYER_EFFECT_HPP
#define IPLAYER_EFFECT_HPP

class Player;

// 把「PowerUp 撿到的瞬間對玩家做了什麼」抽出來，PowerUp 不再為每種效果寫一個 subclass。
// 新增一種效果只需 (a) 新增一個 IPlayerEffect 子類 (b) 在 LootTable 註冊
// — 不必再動 PowerUp、Interactable.hpp 與 Player 公開介面。
class IPlayerEffect {
public:
    virtual ~IPlayerEffect() = default;
    virtual void Apply(Player& player) = 0;

    // 撿到時 log 用的人類可讀名 (e.g. "SPEED_UP"、"BOMB_UP")。
    virtual const char* GetLogName() const = 0;
};

#endif
