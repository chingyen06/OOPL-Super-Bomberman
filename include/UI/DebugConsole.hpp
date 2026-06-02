#ifndef DEBUGCONSOLE_HPP
#define DEBUGCONSOLE_HPP

#include <memory>
#include <vector>

#include "Util/Renderer.hpp"

class Player;
class Spirit;
class InteractableManager;
class CheatManager;
class SaveData;
class BombManager;
class TurretManager;
class DefenderWeaponSystem;

// F3 debug 主控台 (自 GameSession 抽出，SRP)：ImGui debug 視圖 + 控制鈕，不持有遊戲狀態。
// 協作者每幀由 GameSession 明確傳入 Render (依賴外顯，不用 friend)。
class DebugConsole {
public:
    // gameTimeTicks 以參考傳入：+30s / -30s / End now 會就地修改它。
    void Render(int level, int& gameTimeTicks, float fps,
                InteractableManager& interactables, SaveData* profile,
                std::vector<std::shared_ptr<Player>>& players,
                std::vector<std::shared_ptr<Spirit>>& spirits,
                Player* humanPlayer1, Player* humanPlayer2,
                CheatManager& cheat, BombManager& bombs, TurretManager& turrets,
                DefenderWeaponSystem& weapon, Util::Renderer& root);

    bool ShowDanger() const { return m_ShowDanger; }    // 是否疊危險地圖紅塊
    bool FreezeTimer() const { return m_FreezeTimer; }  // 凍結倒數 (debug)

private:
    bool m_ShowDanger  = true;
    bool m_FreezeTimer = false;
};

#endif
