#pragma once

#include "model/GridPos.hpp"
#include "model/components/CommonComponents.hpp"

#include <algorithm>
#include <cmath>

// Geometry of a square footprint in logical tiles. Systems that need the footprint centre or its
// distance to a position use these helpers instead of recomputing bounds by hand.
namespace model {

// Tile at the centre of a square footprint (a 2x2 footprint at (10, 10) is centred at (11, 11)).
[[nodiscard]] constexpr GridPos footprintCenter(const components::Footprint& footprint) noexcept
{
    return GridPos{footprint.origin.x + footprint.size / 2, footprint.origin.y + footprint.size / 2};
}

// Footprint rectangle in continuous grid coordinates.
struct FootprintBounds {
    float left;
    float top;
    float right;
    float bottom;

    [[nodiscard]] constexpr float centerX() const noexcept { return (left + right) / 2.0F; }
    [[nodiscard]] constexpr float centerY() const noexcept { return (top + bottom) / 2.0F; }
};

[[nodiscard]] constexpr FootprintBounds footprintBounds(const components::Footprint& footprint) noexcept
{
    const float left = static_cast<float>(footprint.origin.x);
    const float top = static_cast<float>(footprint.origin.y);
    const float size = static_cast<float>(footprint.size);
    return FootprintBounds{left, top, left + size, top + size};
}

// Euclidean distance from a point to the closest point of the footprint rectangle.
[[nodiscard]] inline float distanceToFootprint(float x, float y, const FootprintBounds& bounds) noexcept
{
    const float nearestX = std::clamp(x, bounds.left, bounds.right);
    const float nearestY = std::clamp(y, bounds.top, bounds.bottom);
    return std::hypot(x - nearestX, y - nearestY);
}

} // namespace model
