#ifndef BOMB_HPP
#define BOMB_HPP

#include "Util/GameObject.hpp"
#include "Util/Image.hpp"
#include <memory>

class Player;
class LevelManager;
class BombManager;
class InteractableManager;

class Bomb : public Util::GameObject {
public:
    enum class State { COUNTDOWN, DONE };

    Bomb(int gridX, int gridY, int firepower, int ownerID);

    void Update(const LevelManager& levelManager, const BombManager& bombManager, const InteractableManager& interactableManager);

    State GetState() const { return m_State; }
    int GetGridX() const { return m_GridX; }
    int GetGridY() const { return m_GridY; }
    int GetFirepower() const { return m_Firepower; }

    int GetOwnerID() const { return m_OwnerID; }
    int RemainingFuse() const { return m_Tick; }  // 距離引爆還有幾 frame (m_Tick 由 kFuseFrames 倒數至 0)

	void ForceDetonate() { m_State = State::DONE; }  // Force detonation (used for chain reactions when another bomb's fire reaches this one)

private:
    int m_GridX;
    int m_GridY;
    int m_Firepower;
    int m_Tick;
    State m_State;
    glm::vec2 m_Pos;

    int m_OwnerID;
};

#endif