#ifndef GRIDCOORD_HPP
#define GRIDCOORD_HPP

#include <cmath>
#include "glm/vec2.hpp"

// Centralised grid<->pixel conversions and map-size constants.
// Eliminates the (gx - 12) * 32.0f and (8 - gy) * 32.0f magic numbers
// that were previously scattered across entity constructors.
namespace GridCoord {

    inline constexpr int   kMapWidth    = 25;      // tile columns
    inline constexpr int   kMapHeight   = 17;      // tile rows
    inline constexpr float kTileSize    = 32.0f;   // pixels per tile
    inline constexpr int   kOriginGridX = 12;      // gridX that maps to pixel x = 0
    inline constexpr int   kOriginGridY = 8;       // gridY that maps to pixel y = 0 (y axis inverted)

    // ---------- Grid -> Pixel ----------
    inline float ToPixelX(int gx) {
        return (gx - kOriginGridX) * kTileSize;
    }
    inline float ToPixelY(int gy) {
        return (kOriginGridY - gy) * kTileSize;
    }
    inline glm::vec2 ToPixel(int gx, int gy) {
        return { ToPixelX(gx), ToPixelY(gy) };
    }

    // ---------- Pixel -> Grid (nearest tile centre) ----------
    inline int ToGridX(float px) {
        return static_cast<int>(std::round(px / kTileSize)) + kOriginGridX;
    }
    inline int ToGridY(float py) {
        return kOriginGridY - static_cast<int>(std::round(py / kTileSize));
    }

    // ---------- Bounds check ----------
    inline bool InBounds(int gx, int gy) {
        return gx >= 0 && gx < kMapWidth && gy >= 0 && gy < kMapHeight;
    }
}

#endif
