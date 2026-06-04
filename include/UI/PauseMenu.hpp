#ifndef PAUSEMENU_HPP
#define PAUSEMENU_HPP

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "UI/ButtonRow.hpp"
#include "UI/UIGroup.hpp"
#include "Util/Color.hpp"
#include "Util/Renderer.hpp"

// 暫停選單：半透明變暗整個畫面 + 右側面板 + 可上下選擇的按鈕。
//
// 按鈕列的共用機制收在基底 ButtonRow；本類只加上「面板裝飾 (變暗 / 面板 / 標題)」
// 與「callback 確認」語意。App 只透過 Show / Hide / Update 操作，建構選項時傳入 callback。
//
// 注意：渲染器的正交投影 near/far = [-100, 100]，z-index 超過 100 會被裁掉不繪製，
// 因此所有圖層的 z 都必須 <= 100 (見下方常數)。
class PauseMenu : public ButtonRow {
public:
    // 載入底圖與建立 overlay / 面板 / 標題 (需在 GL context 就緒後呼叫，例如 App::Start)。
    void Init();

    // 依序由上往下排列；onSelect 在該選項被確認時呼叫。
    void AddOption(const std::string& text, std::function<void()> onSelect);

    // 動態改寫選項文字 (例如作弊模式「開 / 關」切換時刷新標籤)。
    void SetOptionLabel(int index, const std::string& text) { SetLabel(index, text); }

    // 啟用/停用某選項：停用時變灰、導覽會略過、確認時不觸發 (例如玩家2非人類時禁用 P2 作弊)。
    void SetOptionEnabled(int index, bool enabled);

    void Show(Util::Renderer& root);
    void Hide(Util::Renderer& root);
    void Update();  // 處理上下移動與確認

    bool IsVisible() const { return m_IsVisible; }

protected:
    // 暫停面板為深色底，一般字用白色 (選取 / 停用沿用基底的白 / 灰)。
    Util::Color LabelColorNormal() const override { return Util::Color::FromName(Util::Colors::WHITE); }

private:
    // ---- 版面常數 (畫面中心為原點、+y 朝上、1280x720) ----
    static constexpr float kPanelCenterX = 460.0f;  // 右側面板中心 x
    static constexpr float kTitleY       = 270.0f;
    static constexpr float kFirstOptionY = 170.0f;
    static constexpr float kOptionStepY  = 75.0f;
    static constexpr float kLabelXNudge  = 5.0f;    // 文字略往右 (與其他按鈕一致)

    // ---- 圖層 (全部 <= 100，否則被投影裁掉) ----
    static constexpr float kDimZ    = 97.0f;
    static constexpr float kPanelZ  = 98.0f;
    static constexpr float kButtonZ = 99.0f;
    static constexpr float kTextZ   = 100.0f;

    bool m_IsVisible = false;

    UIGroup m_Chrome;  // 變暗 / 面板 / 標題 (與按鈕一起進出場景；用 UIGroup 整批管理)

    std::vector<std::function<void()>> m_Callbacks;
};

#endif
