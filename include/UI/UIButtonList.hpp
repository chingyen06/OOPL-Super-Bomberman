#ifndef UIBUTTONLIST_HPP
#define UIBUTTONLIST_HPP

#include <string>

#include "UI/ButtonRow.hpp"
#include "Util/Renderer.hpp"

// 可重用的「一排可選按鈕」元件，供各選單畫面共用。按鈕列的共用機制 (容器 / 底圖 /
// 導覽 / 高亮 / 配色) 已上移至基底 ButtonRow；本類只定義自己的兩個特性：
//   (1) 彈性版面：由 (startX, startY) 起，每項位移 (stepX, stepY) — 可直/可橫排。
//   (2) 確認語意：Update() 回傳本幀被 SPACE 確認的索引 (否則 -1)，由擁有它的 state
//       在自己的 OnUpdate 末端執行轉場 —— 避免 callback 在 Update() 途中銷毀擁有者
//       (連同本元件) 而 use-after-free。
class UIButtonList : public ButtonRow {
public:
    // normalImg / selectedImg：項目底圖 (一般 / 選取)，於 GL context 就緒後呼叫。
    void Init(const std::string& normalImg, const std::string& selectedImg);

    void AddItem(const std::string& text)                 { AddButton(text, kButtonZ, kLabelZ); }
    void SetItemLabel(int index, const std::string& text) { SetLabel(index, text); }
    void SetItemEnabled(int index, bool enabled)          { SetButtonEnabled(index, enabled); }  // 停用項：變灰、無法選取
    void SetSelected(int index);                          // 指定目前選取項 (需為可選)
    void SetHighlight(bool on);                           // 是否顯示選取高亮 (焦點移出此列表時關掉)

    // 從 (startX, startY) 開始，每個項目位移 (stepX, stepY)。
    void Show(Util::Renderer& root, float startX, float startY, float stepX, float stepY);
    void Hide(Util::Renderer& root);

    int  Update();  // 導覽 + 確認；回傳被確認的索引，否則 -1

    bool IsVisible() const { return m_Visible; }

private:
    static constexpr float kButtonZ = 20.0f;
    static constexpr float kLabelZ  = 30.0f;
    // 文字 (Util::Text) 以中心對齊；視覺上略往右一點較順眼 (與暫停選單 / 確認框一致)。
    static constexpr float kLabelXNudge = 5.0f;
    static constexpr float kLabelYNudge = 1.0f;

    bool m_Visible = false;
};

#endif
