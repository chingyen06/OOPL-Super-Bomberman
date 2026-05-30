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
class CheatManager {
public:
    // human 可能為 nullptr (例如尚未載入關卡)；此時只處理開關，不套用增益。
    void Update(Player* human);

    // 外部 (例如暫停選單) 直接切換作弊模式總開關。
    void Toggle() { m_Enabled = !m_Enabled; }

    bool IsEnabled() const { return m_Enabled; }

private:
    bool m_Enabled = false;      // 作弊模式總開關 (預設關閉)
    bool m_PrevEnabled = false;  // 上一幀狀態 (偵測「關閉那一刻」以立即解除加速)
};

#endif
