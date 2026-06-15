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

        // 開關狀態每幀同步給玩家：
        //   - SetGodMode  ：關閉時解除無敵。
        //   - SetCheatStats：以覆寫方式拉滿炸彈數 / 火力，關閉的瞬間即自動還原 —
        //     不需 off-edge 特別處理，也不會吃掉作弊期間正常拿到的道具。
        human->SetGodMode(m_Enabled[i]);
        human->SetCheatStats(m_Enabled[i]);

        if (m_Enabled[i]) {
            human->ActivateSpeedBoost();   // 每幀刷新計時器 → 持續加速 (重生後仍維持)
        } else if (justDisabled) {
            // 加速是計時型增益 (非覆寫)，須在關閉那一刻主動取消；只做一次，
            // 避免誤刪正常吃到的加速道具。
            human->CancelSpeedBoost();
        }
    }
}
