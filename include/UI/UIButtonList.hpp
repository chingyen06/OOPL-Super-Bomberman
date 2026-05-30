#ifndef UIBUTTONLIST_HPP
#define UIBUTTONLIST_HPP

#include <memory>
#include <string>
#include <vector>

#include "UI/UIImage.hpp"
#include "UI/UIText.hpp"
#include "Util/Image.hpp"
#include "Util/Renderer.hpp"

// 可重用的「一排可選按鈕」元件，供各選單畫面共用 (避免重複)。
//
// 由上往下 (stepX=0) 或由左往右 (stepY=0) 排列；每個項目有一張一般底圖與一張
// 選取底圖，選取項以橘色底圖標示。導覽：↑←/WA 上一個、↓→/SD 下一個 (略過停用項)。
//
// 確認回傳設計 (而非 callback)：Update() 回傳本幀被 SPACE 確認的項目索引 (否則 -1)，
// 由擁有本元件的 state 在它自己的 OnUpdate 末端執行轉場 —— 避免 callback 在 Update()
// 執行途中把擁有者 state (連同本元件) 銷毀而造成 use-after-free。
class UIButtonList {
public:
    // normalImg / selectedImg：項目底圖 (一般 / 選取)，於 GL context 就緒後呼叫。
    void Init(const std::string& normalImg, const std::string& selectedImg);

    void AddItem(const std::string& text);
    void SetItemLabel(int index, const std::string& text);
    void SetItemEnabled(int index, bool enabled);  // 停用項：變灰、無法選取
    void SetSelected(int index);                   // 指定目前選取項 (需為可選)

    // 從 (startX, startY) 開始，每個項目位移 (stepX, stepY)。
    void Show(Util::Renderer& root, float startX, float startY, float stepX, float stepY);
    void Hide(Util::Renderer& root);

    int  Update();  // 導覽 + 確認；回傳被確認的索引，否則 -1

    int  Selected() const { return m_Selected; }
    bool IsVisible() const { return m_Visible; }

private:
    void UpdateCursor();
    int  StepToEnabled(int from, int dir) const;  // 朝 dir 找下一個可選項 (含環繞)

    static constexpr float kButtonZ = 20.0f;
    static constexpr float kLabelZ  = 30.0f;
    // 文字貼圖右側有留白 → 往右補正才水平置中；垂直只需微調 (量測值)。
    static constexpr float kLabelXNudge = 7.0f;
    static constexpr float kLabelYNudge = 1.0f;

    bool m_Visible = false;
    int  m_Selected = 0;

    std::shared_ptr<Util::Image> m_NormalImg;
    std::shared_ptr<Util::Image> m_SelectedImg;

    std::vector<std::string>              m_Texts;
    std::vector<std::shared_ptr<UIImage>> m_Buttons;
    std::vector<std::shared_ptr<UIText>>  m_Labels;
    std::vector<bool>                     m_Enabled;
};

#endif
