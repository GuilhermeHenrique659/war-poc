#pragma once

#include "view/IsoProjection.hpp"

#include <raylib.h>

// Shared unit geometry: the view draws with it, the controller picks with it.
// Sizes are the unit's square footprint in tiles per side.
namespace view::unit_layout {

inline constexpr float kHitBelow = 6.0f;

// Ground point under the unit's center, in world pixels. Positions are continuous grid coordinates.
[[nodiscard]] inline Vector2 groundPoint(float gx, float gy)
{
    return iso::gridToWorld(gx, gy);
}

// Sprite height above the ground point, in world pixels.
[[nodiscard]] inline float spriteHeight(int size)
{
    return size <= 1 ? 38.0f : 34.0f * static_cast<float>(size);
}

[[nodiscard]] inline float halfWidth(int size)
{
    return size <= 1 ? 13.0f : 0.4f * iso::kTileWidth * static_cast<float>(size);
}

// Clickable area around the unit sprite, in world pixels.
[[nodiscard]] inline Rectangle hitBounds(float gx, float gy, int size)
{
    const Vector2 ground = groundPoint(gx, gy);
    const float half = halfWidth(size);
    const float height = spriteHeight(size);
    return Rectangle{ground.x - half, ground.y - height, 2.0f * half, height + kHitBelow};
}

} // namespace view::unit_layout
