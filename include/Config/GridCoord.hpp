#ifndef GRIDCOORD_HPP
#define GRIDCOORD_HPP

#include <cmath>
#include "glm/vec2.hpp"

// Centralised grid<->pixel conversions and map-size constants.
// Eliminates the (gx - 12) * 32.0f and (8 - gy) * 32.0f magic numbers
// that were previously scattered across entity constructors.
//
// 傳統 OOP：以 class + static 成員/方法 取代 namespace。用法 GridCoord::ToPixel(...) 不變。
class GridCoord {
public:
    static constexpr int   kMapWidth    = 25;      // tile columns
    static constexpr int   kMapHeight   = 17;      // tile rows
    static constexpr float kTileSize    = 32.0f;   // pixels per tile
    static constexpr int   kOriginGridX = 12;      // gridX that maps to pixel x = 0
    static constexpr int   kOriginGridY = 8;       // gridY that maps to pixel y = 0 (y axis inverted)

    // ---------- Grid -> Pixel ----------
    static float ToPixelX(int gx) {
        return (gx - kOriginGridX) * kTileSize;
    }
    static float ToPixelY(int gy) {
        return (kOriginGridY - gy) * kTileSize;
    }
    static glm::vec2 ToPixel(int gx, int gy) {
        return { ToPixelX(gx), ToPixelY(gy) };
    }

    // ---------- Pixel -> Grid (nearest tile centre) ----------
    static int ToGridX(float px) {
        return static_cast<int>(std::round(px / kTileSize)) + kOriginGridX;
    }
    static int ToGridY(float py) {
        return kOriginGridY - static_cast<int>(std::round(py / kTileSize));
    }

    // ---------- Bounds check ----------
    static bool InBounds(int gx, int gy) {
        return gx >= 0 && gx < kMapWidth && gy >= 0 && gy < kMapHeight;
    }

private:
    GridCoord() = default;  // 純靜態工具類，不需實例化
};

#endif
