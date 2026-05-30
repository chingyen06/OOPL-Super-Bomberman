#ifndef GAMETYPES_HPP
#define GAMETYPES_HPP

#include <array>

enum class Direction { UP, DOWN, LEFT, RIGHT };

enum class Team { ATTACKER, DEFENDER };

// 4 cardinal directions as (dx, dy) grid offsets. Same ordering as Direction enum.
class GridOffset { public: int dx; int dy; };
inline constexpr std::array<GridOffset, 4> kCardinalOffsets = {{
    { 0, -1}, // UP
    { 0,  1}, // DOWN
    {-1,  0}, // LEFT
    { 1,  0}, // RIGHT
}};

#endif
