#ifndef SELECTABLELIST_HPP
#define SELECTABLELIST_HPP

#include <vector>

// 一排「可用鍵盤導覽」的選項共用基底 (抽自 UIButtonList 與 PauseMenu 中重複的實作)。
// 負責：目前選取索引、各項啟用旗標、跳過停用項的環繞步進、方向鍵 / WASD 導覽輸入。
// 至於視覺呈現 (選取換底圖、停用變灰…) 由子類覆寫 UpdateCursor() 各自定義。
//   - 傳統 OOP：以基底類別承載共用狀態與行為，子類只擴充差異 (消除重複設計)。
class SelectableList {
public:
    virtual ~SelectableList() = default;

    int Selected() const { return m_Selected; }

protected:
    // 朝 dir (+1 / -1) 找下一個「啟用」項，含環繞；全部停用則維持原索引。
    int StepEnabled(int from, int dir) const;

    // 讀方向鍵 + WASD：回 -1 (上一個) / +1 (下一個) / 0 (本幀無導覽輸入)。
    static int ReadNavStep();

    // 依導覽輸入移動選取項；有移動時呼叫 UpdateCursor()。回傳是否真的有移動。
    bool MoveSelection();

    // 子類定義「選取 / 停用」狀態如何反映到畫面上。
    virtual void UpdateCursor() = 0;

    int               m_Selected = 0;
    std::vector<bool> m_Enabled;  // 與子類的項目容器 1:1 對應 (AddItem / AddOption 時 push)
};

#endif
