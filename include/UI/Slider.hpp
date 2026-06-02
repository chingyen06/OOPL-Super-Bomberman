#ifndef UI_SLIDER_HPP
#define UI_SLIDER_HPP

#include <functional>
#include <memory>
#include <vector>

#include "UI/UIImage.hpp"
#include "UI/UIText.hpp"
#include "Util/Image.hpp"
#include "Util/Renderer.hpp"

// 音量用的分段式滑桿 (仿原版)：一排小區塊，已填的亮、未填的暗 + 右側數值 (0..100)。
// 以鍵盤調整 (Adjust ±delta)，值改變時呼叫 OnChange。
// 自行管理自己的圖層，由擁有者在 Show/Hide 掛上/移除。
class Slider {
public:
    static constexpr int kSegments = 20;  // 區塊數 (每格 5%)

    void Show(Util::Renderer& root, float centerX, float y, float trackW, float z);
    void Hide(Util::Renderer& root);

    void SetValue(int percent);          // clamp 0..100，更新外觀 (不觸發 OnChange)
    int  Value() const { return m_Value; }
    void SetFocused(bool focused);       // 聚焦時數值字較亮 / 滑塊放大
    void SetOnChange(std::function<void(int)> cb) { m_OnChange = std::move(cb); }

    void Adjust(int delta);              // 鍵盤：±delta 並觸發 OnChange

private:
    void UpdateVisual();
    float Left() const { return m_CenterX - m_TrackW * 0.5f; }

    // 取代原本散在 .cpp 匿名 namespace 的常數與自由函式 (傳統 OOP：收進類別)
    static constexpr float kKeycapW = 60.0f, kKeycapH = 42.0f;  // keycap 貼圖原始尺寸
    static constexpr float kSegH    = 26.0f;                    // 區塊高
    static int Clamp100(int v);                                 // clamp 0..100
    static int FilledCount(int value);                          // value(%) → 已填區塊數

    float m_CenterX = 0.0f, m_Y = 0.0f, m_TrackW = 240.0f, m_Z = 21.0f;
    int   m_Value = 0;
    bool  m_Focused = false;

    std::vector<std::shared_ptr<UIImage>> m_Segments;    // 一排區塊
    std::shared_ptr<Util::Image> m_SegOn, m_SegOff;      // 亮 / 暗 區塊貼圖
    std::shared_ptr<UIText>  m_ValueText;
    std::function<void(int)> m_OnChange;
};

#endif
