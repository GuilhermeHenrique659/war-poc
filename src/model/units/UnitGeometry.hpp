#pragma once

#include "model/GridPos.hpp"
#include "model/components/CommonComponents.hpp"

#include <cmath>

namespace model::unit_geometry {

// Keeps a unit standing exactly on its square from touching the neighbor tiles.
inline constexpr float kCoverEpsilon = 0.01F;

[[nodiscard]] inline float halfSize(int size)
{
    return static_cast<float>(size) / 2.0F;
}

[[nodiscard]] inline GridPos originOf(const components::Position& position, int size)
{
    const float offset = halfSize(size) - 0.5F;
    return GridPos{static_cast<int>(std::floor(position.x - offset)),
        static_cast<int>(std::floor(position.y - offset))};
}

[[nodiscard]] inline components::Position centerOf(GridPos origin, int size)
{
    return components::Position{static_cast<float>(origin.x) + halfSize(size),
        static_cast<float>(origin.y) + halfSize(size)};
}

[[nodiscard]] inline bool covers(const components::Position& position, int size, GridPos tile)
{
    const float half = halfSize(size) - kCoverEpsilon;
    const float x = static_cast<float>(tile.x);
    const float y = static_cast<float>(tile.y);
    return x + 1.0F > position.x - half && x < position.x + half && y + 1.0F > position.y - half
        && y < position.y + half;
}

template <typename Func>
bool anyTileOf(GridPos origin, int size, Func&& func)
{
    for (int dy = 0; dy < size; ++dy) {
        for (int dx = 0; dx < size; ++dx) {
            if (func(GridPos{origin.x + dx, origin.y + dy})) {
                return true;
            }
        }
    }
    return false;
}

// Origin that centers a square of `size` on the tile.
[[nodiscard]] inline GridPos centeredOrigin(GridPos tile, int size)
{
    return GridPos{tile.x - (size - 1) / 2, tile.y - (size - 1) / 2};
}

} // namespace model::unit_geometry
