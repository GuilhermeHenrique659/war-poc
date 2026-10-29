#pragma once

#include <array>
#include <cmath>
#include <cstddef>

namespace model {

// Compass direction in grid space. Grid +x is East and +y is South. The View projects it to the
// screen-facing sprite direction, because the isometric projection rotates the axes.
enum class Direction { North, NorthEast, East, SouthEast, South, SouthWest, West, NorthWest };

// Picks the grid direction of a movement delta. When there is no movement at all, the previous
// direction is kept.
[[nodiscard]] inline Direction directionFromGridMovement(
    float gridDx, float gridDy, Direction fallback = Direction::South)
{
    if (gridDx == 0.0F && gridDy == 0.0F) {
        return fallback;
    }

    // Clockwise from north, matching the order of Direction.
    constexpr std::array<Direction, 8> kDirections{Direction::North, Direction::NorthEast,
        Direction::East, Direction::SouthEast, Direction::South, Direction::SouthWest,
        Direction::West, Direction::NorthWest};
    constexpr float kOctant = 3.14159265358979323846F / 4.0F;
    const float clockwiseFromNorth = std::atan2(gridDx, -gridDy);
    const int octant = static_cast<int>(std::lround(clockwiseFromNorth / kOctant));
    const int wrapped = ((octant % 8) + 8) % 8;
    return kDirections[static_cast<std::size_t>(wrapped)];
}

} // namespace model
