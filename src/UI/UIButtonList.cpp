#include "UI/UIButtonList.hpp"

#include "Util/Input.hpp"
#include "Util/Keycode.hpp"

void UIButtonList::Init(const std::string& normalImg, const std::string& selectedImg) {
    SetButtonImages(std::make_shared<Util::Image>(normalImg),
                    std::make_shared<Util::Image>(selectedImg));
}

void UIButtonList::SetSelected(int index) {
    if (index < 0 || index >= Count()) return;
    if (!m_Enabled[index]) return;
    m_Selected = index;
    if (m_Visible) UpdateCursor();
}

void UIButtonList::SetHighlight(bool on) {
    if (m_Highlight == on) return;
    m_Highlight = on;
    if (m_Visible) UpdateCursor();
}

void UIButtonList::Show(Util::Renderer& root, float startX, float startY, float stepX, float stepY) {
    if (m_Visible) return;
    m_Visible = true;

    for (int i = 0; i < Count(); i++) {
        const float x = startX + static_cast<float>(i) * stepX;
        const float y = startY + static_cast<float>(i) * stepY;
        m_Buttons[i]->SetPosition(x, y);
        m_Labels[i]->SetPosition(x + kLabelXNudge, y - kLabelYNudge);
    }
    AttachButtons(root);

    m_Selected = StepEnabled(-1, +1);  // 第一個可選項
    UpdateCursor();
}

void UIButtonList::Hide(Util::Renderer& root) {
    if (!m_Visible) return;
    m_Visible = false;
    DetachButtons(root);
}

int UIButtonList::Update() {
    if (!m_Visible || Count() == 0) return -1;

    MoveSelection();  // 方向鍵 / WASD 導覽 (共用自 SelectableList)

    if (Util::Input::IsKeyUp(Util::Keycode::SPACE) && m_Enabled[m_Selected]) {
        return m_Selected;
    }
    return -1;
}
