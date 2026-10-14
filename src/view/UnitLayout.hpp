#pragma once

#include "view/IsoProjection.hpp"

#include <raylib.h>

// Shared unit geometry: the view draws with it, the controller picks with it.
namespace view::unit_layout {

inline constexpr float kHitHalfWidth = 13.0f;
inline constexpr float kHitHeight = 38.0f; // Sprite height above the ground point.
inline constexpr float kHitBelow = 6.0f;

// Ground point of a unit, in world pixels. Positions are continuous grid coordinates.
[[nodiscard]] inline Vector2 groundPoint(float gx, float gy)
{
    return iso::gridToWorld(gx, gy);
}

// Clickable area around the unit sprite, in world pixels.
[[nodiscard]] inline Rectangle hitBounds(float gx, float gy)
{
    const Vector2 ground = groundPoint(gx, gy);
    return Rectangle{ground.x - kHitHalfWidth, ground.y - kHitHeight, 2.0f * kHitHalfWidth, kHitHeight + kHitBelow};
}

} // namespace view::unit_layout
