#include "UI/ConfirmDialog.hpp"

#include "Util/Color.hpp"
#include "Util/Input.hpp"
#include "Util/Keycode.hpp"

void ConfirmDialog::Init() {
    m_BtnNormal   = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/btn.png");
    m_BtnSelected = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/btn_sel.png");
}

void ConfirmDialog::Show(Util::Renderer& root, const std::string& question) {
    if (m_Visible) return;
    m_Visible = true;
    m_Sel = 1;  // 預設「否」

    m_Dim = std::make_shared<UIImage>(RESOURCE_DIR"/Image/pause_dim.png", 0.0f, 0.0f, kZDim);
    m_Dim->SetFullScreen();
    m_Panel = std::make_shared<UIImage>(RESOURCE_DIR"/Image/dlg_panel.png", 0.0f, -10.0f, kZPanel);
    m_TitleBar = std::make_shared<UIImage>(RESOURCE_DIR"/Image/dlg_titlebar.png", 0.0f, 195.0f, kZPanel);
    m_Title = std::make_shared<UIText>("確認", 0.0f, 195.0f, kZText, Util::Color::FromName(Util::Colors::WHITE));
    m_Question = std::make_shared<UIText>(question, 0.0f, 30.0f, kZText, Util::Color::FromName(Util::Colors::BLACK));

    m_YesBtn = std::make_shared<UIImage>(-170.0f, -150.0f, kZBtn);  m_YesBtn->SetDrawable(m_BtnNormal);
    m_NoBtn  = std::make_shared<UIImage>( 170.0f, -150.0f, kZBtn);  m_NoBtn->SetDrawable(m_BtnNormal);
    // 文字略往右一點 (與其他按鈕一致)；按鈕底圖仍在 ±170。
    m_YesLabel = std::make_shared<UIText>("是", -165.0f, -150.0f, kZText, Util::Color::FromName(Util::Colors::BLACK));
    m_NoLabel  = std::make_shared<UIText>("否",  175.0f, -150.0f, kZText, Util::Color::FromName(Util::Colors::BLACK));

    for (auto& n : { m_Dim, m_Panel, m_TitleBar, m_YesBtn, m_NoBtn }) root.AddChild(n);
    for (auto& n : { m_Title, m_Question, m_YesLabel, m_NoLabel })    root.AddChild(n);
    UpdateCursor();
}

void ConfirmDialog::Hide(Util::Renderer& root) {
    if (!m_Visible) return;
    m_Visible = false;
    for (auto& n : { m_Dim, m_Panel, m_TitleBar, m_YesBtn, m_NoBtn }) root.RemoveChild(n);
    for (auto& n : { m_Title, m_Question, m_YesLabel, m_NoLabel })    root.RemoveChild(n);
}

void ConfirmDialog::UpdateCursor() {
    const bool yesSel = (m_Sel == 0);
    m_YesBtn->SetDrawable(yesSel ? m_BtnSelected : m_BtnNormal);
    m_NoBtn->SetDrawable(yesSel ? m_BtnNormal : m_BtnSelected);
    m_YesLabel->SetColor(yesSel ? Util::Color::FromName(Util::Colors::WHITE) : Util::Color::FromName(Util::Colors::BLACK));
    m_NoLabel->SetColor(yesSel ? Util::Color::FromName(Util::Colors::BLACK) : Util::Color::FromName(Util::Colors::WHITE));
}

ConfirmDialog::Result ConfirmDialog::Update() {
    if (!m_Visible) return Result::Pending;
    using K = Util::Keycode;

    if (Util::Input::IsKeyUp(K::LEFT) || Util::Input::IsKeyUp(K::A) ||
        Util::Input::IsKeyUp(K::RIGHT) || Util::Input::IsKeyUp(K::D)) {
        m_Sel ^= 1;
        UpdateCursor();
    }
    if (Util::Input::IsKeyUp(K::X) || Util::Input::IsKeyUp(K::ESCAPE)) return Result::No;  // 取消視為否
    if (Util::Input::IsKeyUp(K::SPACE) || Util::Input::IsKeyUp(K::RETURN)) {
        return (m_Sel == 0) ? Result::Yes : Result::No;
    }
    return Result::Pending;
}
