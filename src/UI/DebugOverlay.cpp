#include "DebugOverlay.hpp"

#include "Util/Color.hpp"

// 左上資訊面板版面 (UIText 為置中錨點，故每行置中於 kLineX；行文字保持精簡以免被左緣裁切)。
static constexpr float kLineX = -430.0f;
static constexpr float kTopY  = 300.0f;
static constexpr float kStep  = 30.0f;

static Util::Color DebugColor() { return Util::Color(90, 255, 90); }  // 經典除錯綠

void DebugOverlay::Update(Util::Renderer& root,
                          const std::vector<std::string>& lines,
                          const std::vector<std::pair<float, float>>& dangerCells) {
    // ---- 資訊面板 ----
    if (m_Enabled) {
        for (size_t i = 0; i < lines.size(); ++i) {
            if (i >= m_Lines.size()) {
                auto t = std::make_shared<UIText>("-", kLineX, kTopY, 96.0f, DebugColor());
                root.AddChild(t);
                m_Lines.push_back(t);
            }
            m_Lines[i]->SetText(lines[i]);
            m_Lines[i]->SetPosition(kLineX, kTopY - static_cast<float>(i) * kStep);
            m_Lines[i]->SetVisible(true);
        }
        for (size_t i = lines.size(); i < m_Lines.size(); ++i) m_Lines[i]->SetVisible(false);
    }
    else {
        for (auto& t : m_Lines) t->SetVisible(false);
    }

    // ---- 危險格視覺化 ----
    if (m_Enabled) {
        for (size_t i = 0; i < dangerCells.size(); ++i) {
            if (i >= m_Danger.size()) {
                auto sq = std::make_shared<UIImage>(RESOURCE_DIR"/Image/dbg_danger.png", 0.0f, 0.0f, 80.0f);
                root.AddChild(sq);
                m_Danger.push_back(sq);
            }
            m_Danger[i]->SetPosition(dangerCells[i].first, dangerCells[i].second);
            m_Danger[i]->SetVisible(true);
        }
        for (size_t i = dangerCells.size(); i < m_Danger.size(); ++i) m_Danger[i]->SetVisible(false);
    }
    else {
        for (auto& s : m_Danger) s->SetVisible(false);
    }
}

void DebugOverlay::Clear(Util::Renderer& root) {
    for (auto& t : m_Lines)  root.RemoveChild(t);
    m_Lines.clear();
    for (auto& s : m_Danger) root.RemoveChild(s);
    m_Danger.clear();
}
