#ifndef PLAYER_EFFECTS_HPP
#define PLAYER_EFFECTS_HPP

#include "Effects/IPlayerEffect.hpp"
#include "Player.hpp"

// 三種既有 PowerUp 效果的具體實作。皆只透過 Player 公開介面修改狀態，
// Player 不需要再為每一種效果開新方法。
class SpeedBoostEffect : public IPlayerEffect {
public:
    void Apply(Player& player) override { player.ActivateSpeedBoost(); }
    const char* GetLogName() const override { return "SPEED_UP"; }
};

class BombUpEffect : public IPlayerEffect {
public:
    void Apply(Player& player) override { player.IncreaseMaxBombs(); }
    const char* GetLogName() const override { return "BOMB_UP"; }
};

class FirepowerUpEffect : public IPlayerEffect {
public:
    void Apply(Player& player) override { player.IncreaseFirepower(); }
    const char* GetLogName() const override { return "FIRE_UP"; }
};

#endif
