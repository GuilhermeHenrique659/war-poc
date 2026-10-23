#pragma once

#include "model/GridPos.hpp"

#include <raylib.h>

#include <cmath>

// Grid <-> world-pixel isometric projection. The view draws with it, the controller picks with it.
namespace view::iso {

inline constexpr float kTileWidth = 128.0f;
inline constexpr float kTileHeight = 64.0f;
inline constexpr float kHalfWidth = kTileWidth / 2.0f;
inline constexpr float kHalfHeight = kTileHeight / 2.0f;

// World position of the tile's top corner.
[[nodiscard]] inline Vector2 gridToWorld(float gx, float gy)
{
    return Vector2{(gx - gy) * kHalfWidth, (gx + gy) * kHalfHeight};
}

[[nodiscard]] inline Vector2 gridToWorld(model::GridPos tile)
{
    return gridToWorld(static_cast<float>(tile.x), static_cast<float>(tile.y));
}

[[nodiscard]] inline model::GridPos worldToGrid(Vector2 world)
{
    const float u = world.x / kHalfWidth;
    const float v = world.y / kHalfHeight;
    return model::GridPos{static_cast<int>(std::floor((v + u) / 2.0f)), static_cast<int>(std::floor((v - u) / 2.0f))};
}

// The camera target is the world point shown at the center of the screen.
[[nodiscard]] inline Camera2D makeCamera(Vector2 target, float zoom, int screenWidth, int screenHeight)
{
    Camera2D camera{};
    camera.offset = Vector2{static_cast<float>(screenWidth) / 2.0f, static_cast<float>(screenHeight) / 2.0f};
    camera.target = target;
    camera.rotation = 0.0f;
    camera.zoom = zoom;
    return camera;
}

[[nodiscard]] inline model::GridPos screenToGrid(Vector2 screen, const Camera2D& camera)
{
    return worldToGrid(GetScreenToWorld2D(screen, camera));
}

} // namespace view::iso
