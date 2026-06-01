#include "UI/Slider.hpp"

#include "Util/Color.hpp"

namespace {
constexpr float kKeycapW = 60.0f, kKeycapH = 42.0f;  // keycap 圖原始尺寸
constexpr float kSegH = 26.0f;                        // 區塊顯示高度
int Clamp100(int v) { return v < 0 ? 0 : (v > 100 ? 100 : v); }
int FilledCount(int value) { return (value * Slider::kSegments + 50) / 100; }  // 四捨五入到區塊數
}

void Slider::Show(Util::Renderer& root, float centerX, float y, float trackW, float z) {
    m_CenterX = centerX; m_Y = y; m_TrackW = trackW; m_Z = z;

    m_SegOn  = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/keycap_sel.png");   // 亮 (已填)
    m_SegOff = std::make_shared<Util::Image>(RESOURCE_DIR"/Image/keycap_dark.png");  // 暗 (未填)

    const float left = Left();
    const float segPitch = m_TrackW / static_cast<float>(kSegments);  // 每格中心間距
    const float segW = segPitch * 0.74f;                              // 區塊寬 (留間隙)
    m_Segments.clear();
    for (int i = 0; i < kSegments; ++i) {
        auto seg = std::make_shared<UIImage>(left + (i + 0.5f) * segPitch, y, z);
        seg->SetScale(segW / kKeycapW, kSegH / kKeycapH);
        seg->SetDrawable(m_SegOff);
        root.AddChild(seg);
        m_Segments.push_back(seg);
    }
    m_ValueText = std::make_shared<UIText>("0", left + m_TrackW + 44.0f, y, z + 1.0f,
                                           Util::Color::FromName(Util::Colors::WHITE));
    root.AddChild(m_ValueText);
    UpdateVisual();
}

void Slider::Hide(Util::Renderer& root) {
    for (auto& s : m_Segments) root.RemoveChild(s);
    m_Segments.clear();
    if (m_ValueText) { root.RemoveChild(m_ValueText); m_ValueText.reset(); }
}

void Slider::SetValue(int percent) {
    m_Value = Clamp100(percent);
    UpdateVisual();
}

void Slider::SetFocused(bool focused) {
    m_Focused = focused;
    UpdateVisual();
}

void Slider::Adjust(int delta) {
    const int nv = Clamp100(m_Value + delta);
    if (nv != m_Value) { m_Value = nv; UpdateVisual(); if (m_OnChange) m_OnChange(m_Value); }
}

void Slider::UpdateVisual() {
    const int filled = FilledCount(m_Value);
    for (int i = 0; i < static_cast<int>(m_Segments.size()); ++i) {
        m_Segments[i]->SetDrawable(i < filled ? m_SegOn : m_SegOff);
    }
    if (m_ValueText) {
        m_ValueText->SetText(std::to_string(m_Value));
        m_ValueText->SetColor(m_Focused ? Util::Color::FromName(Util::Colors::WHITE)
                                        : Util::Color::FromName(Util::Colors::GRAY));
    }
}
