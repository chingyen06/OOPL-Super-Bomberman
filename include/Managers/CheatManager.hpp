#ifndef CHEATMANAGER_HPP
#define CHEATMANAGER_HPP

class Player;

// 作弊模式：對「人類玩家」套用增益。
//
// 與 gameplay 其餘部分解耦 —— 只透過 Player 的 public API 操作，不直接觸碰
// 其他 manager 或 Player 的私有狀態 (SRP / DIP)。GameSession 每幀呼叫一次
// Update()，把目前人類控制的玩家傳進來。
//
// 開關由暫停選單透過 Toggle() 切換 (整合在暫停選單內，無獨立熱鍵)。
// 開啟時：永久無敵 + 炸彈數 / 火力拉滿 + 持續加速。
// 支援兩名人類玩家獨立開關：index 0 = 玩家1 (防守)、index 1 = 玩家2 (人類攻擊方)。
class CheatManager {
public:
    static constexpr int kPlayers = 2;

    // p1 / p2 可能為 nullptr (尚未載入關卡、或玩家2非人類)；此時只處理開關旗標。
    void Update(Player* p1, Player* p2);

    void Toggle(int idx)            { if (idx >= 0 && idx < kPlayers) m_Enabled[idx] = !m_Enabled[idx]; }
    bool IsEnabled(int idx) const   { return (idx >= 0 && idx < kPlayers) && m_Enabled[idx]; }

private:
    bool m_Enabled[kPlayers] = { false, false };  // 各玩家作弊開關 (預設關閉)
    bool m_Prev[kPlayers]    = { false, false };  // 上一幀狀態 (偵測「關閉那一刻」以解除加速)
};

#endif
