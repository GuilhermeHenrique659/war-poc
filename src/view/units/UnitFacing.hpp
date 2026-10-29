#pragma once

#include "model/Direction.hpp"

namespace view {

// Screen-facing directions used by the directional unit sprites. The order is clockwise from
// north so the enum value can index the eight sprite sheets directly.
enum class UnitFacing { North, NorthEast, East, SouthEast, South, SouthWest, West, NorthWest };

// Picks the direction a unit appears to walk when its grid position changes by (gridDx, gridDy).
// The grid delta is first projected isometrically, so the result matches what the player sees on
// screen. When there is no movement at all, the previous facing is kept.
[[nodiscard]] UnitFacing facingFromGridMovement(
    float gridDx, float gridDy, UnitFacing fallback = UnitFacing::South);

// Screen-facing sprite direction for the grid direction stored by the model.
[[nodiscard]] UnitFacing facingFromDirection(model::Direction direction);

} // namespace view
