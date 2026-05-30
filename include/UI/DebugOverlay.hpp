#ifndef DEBUGOVERLAY_HPP
#define DEBUGOVERLAY_HPP

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "UI/UIImage.hpp"
#include "UI/UIText.hpp"
#include "Util/Renderer.hpp"

// Debug 模式：F3 切換。啟用時於左上顯示即時數據 (FPS、關卡、時間、實體數、人類玩家狀態)，
// 並把 AI 的「危險地圖」(炸彈/爆炸會掃到的格子) 以半透明紅塊疊在場上做視覺化。
// 與遊戲邏輯解耦 — GameSession 每幀把資料餵進來，這裡只負責顯示。
class DebugOverlay {
public:
    void Toggle() { m_Enabled = !m_Enabled; }
    bool IsEnabled() const { return m_Enabled; }

    // lines       : 左上資訊面板的多行文字
    // dangerCells : 要標紅的格子像素中心座標 (空 = 不顯示)
    void Update(Util::Renderer& root,
                const std::vector<std::string>& lines,
                const std::vector<std::pair<float, float>>& dangerCells);

    void Clear(Util::Renderer& root);

private:
    bool m_Enabled = false;

    std::vector<std::shared_ptr<UIText>>  m_Lines;    // 資訊面板 (依需要成長)
    std::vector<std::shared_ptr<UIImage>> m_Danger;   // 危險格紅塊池 (依需要成長)
};

#endif
