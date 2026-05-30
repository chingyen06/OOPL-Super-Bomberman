#include "CheatManager.hpp"

#include "Player.hpp"

void CheatManager::Update(Player* human) {
    // 開關由暫停選單透過 Toggle() 控制 (不再監聽獨立熱鍵)。
    const bool justDisabled = m_PrevEnabled && !m_Enabled;
    m_PrevEnabled = m_Enabled;

    if (!human) return;

    // 關閉的那一刻立即取消加速 (只在 off-edge 做一次，避免誤刪正常吃到的加速道具)。
    if (justDisabled) human->CancelSpeedBoost();

    // 開關狀態每幀同步給玩家：關閉時 SetGodMode(false) 會解除無敵。
    human->SetGodMode(m_Enabled);

    if (m_Enabled) {
        // 持續套用，重生 (理論上不會發生) 後仍維持滿狀態。
        human->MaxOutBombs();
        human->MaxOutFirepower();
        human->ActivateSpeedBoost();  // 每幀刷新計時器 → 持續加速
    }
}
