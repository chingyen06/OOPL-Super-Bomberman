#include "PauseMenu.hpp"

#include "UI/UIImage.hpp"
#include "UI/UIText.hpp"
#include "Util/Image.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"

void PauseMenu::Init() {
    // 預載按鈕底圖 (一般 / 選取)，之後切換選取狀態只換 drawable，不重讀檔
    SetButtonImages(std::make_shared<Util::Image>(RESOURCE_DIR"/Image/pause_btn.png"),
                    std::make_shared<Util::Image>(RESOURCE_DIR"/Image/pause_btn_sel.png"));

    // 全螢幕變暗 (半透明黑) — 蓋在凍結的遊戲畫面上
    auto dim = std::make_shared<UIImage>(RESOURCE_DIR"/Image/pause_dim.png", 0.0f, 0.0f, kDimZ);
    dim->SetFullScreen();
    m_Chrome.Track(dim);

    // 右側面板 (稍深的半透明) — 原圖 16x16，放大成 360x720
    auto panel = std::make_shared<UIImage>(RESOURCE_DIR"/Image/pause_panel.png", kPanelCenterX, 0.0f, kPanelZ);
    panel->SetScale(360.0f / 16.0f, 720.0f / 16.0f);
    m_Chrome.Track(panel);

    auto title = std::make_shared<UIText>("暫停", kPanelCenterX, kTitleY, kTextZ,
                                          Util::Color::FromName(Util::Colors::WHITE));
    m_Chrome.Track(title);
}

void PauseMenu::AddOption(const std::string& text, std::function<void()> onSelect) {
    const float y = kFirstOptionY - static_cast<float>(Count()) * kOptionStepY;

    const int i = AddButton(text, kButtonZ, kTextZ);
    m_Buttons[i]->SetPosition(kPanelCenterX, y);
    m_Labels[i]->SetPosition(kPanelCenterX + kLabelXNudge, y);  // 文字略往右一點 (與其他按鈕一致)

    m_Callbacks.push_back(std::move(onSelect));
}

void PauseMenu::SetOptionEnabled(int index, bool enabled) {
    SetButtonEnabled(index, enabled);
    if (m_IsVisible) {
        if (!m_Enabled[m_Selected]) m_Selected = StepEnabled(m_Selected, +1);
        UpdateCursor();
    }
}

void PauseMenu::Show(Util::Renderer& root) {
    if (m_IsVisible) return;
    m_IsVisible = true;
    m_Selected = StepEnabled(-1, +1);  // 落在第一個可選項

    m_Chrome.Attach(root);
    AttachButtons(root);

    UpdateCursor();
}

void PauseMenu::Hide(Util::Renderer& root) {
    if (!m_IsVisible) return;
    m_IsVisible = false;

    m_Chrome.Detach(root);
    DetachButtons(root);
}

void PauseMenu::Update() {
    if (!m_IsVisible || Count() == 0) return;

    MoveSelection();  // 方向鍵 / WASD 導覽 (共用自 SelectableList)

    // SPACE = 確認選取的選項 (ENTER 由 GamePausedState 攔截作為「退回去」)。
    if (Util::Input::IsKeyUp(Util::Keycode::SPACE)) {
        // 注意：callback 可能會 TransitionTo 並隱藏本選單，呼叫後立即 return，
        // 不再觸碰任何成員 (與專案其餘 state 轉移慣例一致)。
        if (m_Enabled[m_Selected] && m_Callbacks[m_Selected]) m_Callbacks[m_Selected]();
        return;
    }
}
