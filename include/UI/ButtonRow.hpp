#ifndef BUTTONROW_HPP
#define BUTTONROW_HPP

#include <memory>
#include <string>
#include <vector>

#include "UI/SelectableList.hpp"
#include "UI/UIImage.hpp"
#include "UI/UIText.hpp"
#include "Util/Color.hpp"
#include "Util/Image.hpp"
#include "Util/Renderer.hpp"

// 「一排可用鍵盤導覽的按鈕 (底圖 + 文字標籤)」共用機制 —— 抽自 UIButtonList 與
// PauseMenu 中幾乎一字不差的重複實作 (新增/改字/停用/選取高亮/整批進出場景)。
//   - 傳統 OOP：把共用狀態與行為上移到基底，子類只擴充「彼此刻意不同」的部分
//     (UIButtonList 回傳被確認索引、PauseMenu 觸發 callback；版面與配色各異)。
//   - 標籤配色以虛擬 hook 提供，子類覆寫即可 (OCP)。
class ButtonRow : public SelectableList {
public:
    int Count() const { return static_cast<int>(m_Buttons.size()); }

protected:
    // 設定一般 / 選取底圖 (子類在 Init、GL context 就緒後呼叫)。
    void SetButtonImages(const std::shared_ptr<Util::Image>& normal,
                         const std::shared_ptr<Util::Image>& selected);

    // 新增一個按鈕 (底圖 z = buttonZ、標籤 z = labelZ)，初始位置 (0,0)；回傳新項索引。
    // 座標由子類自行設定 (兩者版面不同：等距排列 vs 固定面板位置)。
    int AddButton(const std::string& text, float buttonZ, float labelZ);

    void SetLabel(int index, const std::string& text);
    void SetButtonEnabled(int index, bool enabled);

    // 把所有按鈕 + 標籤掛上 / 從 root 移除 (子類在 Show / Hide 呼叫)。
    void AttachButtons(Util::Renderer& root);
    void DetachButtons(Util::Renderer& root);

    // SelectableList：依目前選取 / 停用 / 高亮狀態刷新底圖與字色。
    void UpdateCursor() override;

    // 標籤配色 hook (子類可覆寫)。預設：一般黑、選取白、停用灰。
    virtual Util::Color LabelColorNormal()   const { return Util::Color::FromName(Util::Colors::BLACK); }
    virtual Util::Color LabelColorSelected() const { return Util::Color::FromName(Util::Colors::WHITE); }
    virtual Util::Color LabelColorDisabled() const { return Util::Color::FromName(Util::Colors::GRAY); }

    bool m_Highlight = true;  // false → 不顯示選取高亮 (全部維持一般底圖)

    std::shared_ptr<Util::Image> m_BtnNormal;
    std::shared_ptr<Util::Image> m_BtnSelected;

    std::vector<std::string>              m_Texts;
    std::vector<std::shared_ptr<UIImage>> m_Buttons;
    std::vector<std::shared_ptr<UIText>>  m_Labels;
};

#endif
