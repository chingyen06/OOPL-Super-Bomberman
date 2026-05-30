#include "CheatManager.hpp"

#include "Player.hpp"

void CheatManager::Update(Player* p1, Player* p2) {
    Player* targets[kPlayers] = { p1, p2 };
    for (int i = 0; i < kPlayers; ++i) {
        // 開關由暫停選單透過 Toggle() 控制 (不再監聽獨立熱鍵)。
        const bool justDisabled = m_Prev[i] && !m_Enabled[i];
        m_Prev[i] = m_Enabled[i];

        Player* human = targets[i];
        if (!human) continue;

        // 關閉的那一刻立即取消加速 (只在 off-edge 做一次，避免誤刪正常吃到的加速道具)。
        if (justDisabled) human->CancelSpeedBoost();

        // 開關狀態每幀同步給玩家：關閉時 SetGodMode(false) 會解除無敵。
        human->SetGodMode(m_Enabled[i]);

        if (m_Enabled[i]) {
            // 持續套用，重生後仍維持滿狀態。
            human->MaxOutBombs();
            human->MaxOutFirepower();
            human->ActivateSpeedBoost();  // 每幀刷新計時器 → 持續加速
        }
    }
}
