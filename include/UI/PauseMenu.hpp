#ifndef PAUSEMENU_HPP
#define PAUSEMENU_HPP

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "UI/UIImage.hpp"
#include "UI/UIText.hpp"
#include "Util/Image.hpp"
#include "Util/Renderer.hpp"

// 暫停選單：半透明變暗整個畫面 + 右側面板 + 可上下選擇的按鈕。
//
// 視覺與導覽邏輯都封裝在此類別內 (SRP)，App 只透過 Show / Hide / Update 操作，
// 並在建構選項時傳入 callback。按鈕底圖 (一般 / 選取橘色) 在 Init 時預載。
//
// 注意：渲染器的正交投影 near/far = [-100, 100]，z-index 超過 100 會被裁掉不繪製，
// 因此所有圖層的 z 都必須 <= 100 (見下方常數)。
class PauseMenu {
public:
    // 載入底圖與建立 overlay / 面板 / 標題 (需在 GL context 就緒後呼叫，例如 App::Start)。
    void Init();

    // 依序由上往下排列；onSelect 在該選項被確認時呼叫。
    void AddOption(const std::string& text, std::function<void()> onSelect);

    // 動態改寫選項文字 (例如作弊模式「開 / 關」切換時刷新標籤)。
    void SetOptionLabel(int index, const std::string& text);

    // 啟用/停用某選項：停用時變灰、導覽會略過、確認時不觸發 (例如玩家2非人類時禁用 P2 作弊)。
    void SetOptionEnabled(int index, bool enabled);

    void Show(Util::Renderer& root);
    void Hide(Util::Renderer& root);
    void Update();  // 處理上下移動與確認

    bool IsVisible() const { return m_IsVisible; }

private:
    void UpdateCursor();  // 依目前選取項切換按鈕底圖
    int  StepEnabled(int from, int dir) const;  // 朝 dir 找下一個可選 (略過停用)

    // ---- 版面常數 (畫面中心為原點、+y 朝上、1280x720) ----
    static constexpr float kPanelCenterX = 460.0f;  // 右側面板中心 x
    static constexpr float kTitleY       = 270.0f;
    static constexpr float kFirstOptionY = 170.0f;
    static constexpr float kOptionStepY  = 75.0f;

    // ---- 圖層 (全部 <= 100，否則被投影裁掉) ----
    static constexpr float kDimZ    = 97.0f;
    static constexpr float kPanelZ  = 98.0f;
    static constexpr float kButtonZ = 99.0f;
    static constexpr float kTextZ   = 100.0f;

    bool m_IsVisible = false;
    int  m_SelectedIndex = 0;

    std::shared_ptr<UIImage> m_Dim;    // 全螢幕變暗
    std::shared_ptr<UIImage> m_Panel;  // 右側面板
    std::shared_ptr<UIText>  m_Title;  // 「暫停」

    std::shared_ptr<Util::Image> m_BtnNormal;    // 一般按鈕底圖
    std::shared_ptr<Util::Image> m_BtnSelected;  // 選取按鈕底圖 (橘)

    std::vector<std::string>              m_RawTexts;
    std::vector<std::shared_ptr<UIImage>> m_Buttons;
    std::vector<std::shared_ptr<UIText>>  m_Labels;
    std::vector<std::function<void()>>    m_Callbacks;
    std::vector<bool>                     m_Enabled;
};

#endif
