#include "view/units/UnitFacing.hpp"

#include "view/IsoProjection.hpp"

#include <array>
#include <cmath>
#include <cstddef>

namespace view {

namespace {

// Clockwise from north, matching the order of UnitFacing.
constexpr std::array<UnitFacing, 8> kFacingsByOctant{UnitFacing::North, UnitFacing::NorthEast,
    UnitFacing::East, UnitFacing::SouthEast, UnitFacing::South, UnitFacing::SouthWest,
    UnitFacing::West, UnitFacing::NorthWest};

constexpr float kHalfTurn = 3.14159265358979323846F;
constexpr float kOctant = kHalfTurn / 4.0F;

struct GridStep {
    float dx;
    float dy;
};

} // namespace

UnitFacing facingFromGridMovement(float gridDx, float gridDy, UnitFacing fallback)
{
    // Grid motion is projected isometrically before choosing the octant, so a step along a grid
    // axis maps to the diagonal the player actually sees on screen.
    const float screenX = (gridDx - gridDy) * iso::kHalfWidth;
    const float screenY = (gridDx + gridDy) * iso::kHalfHeight;
    if (screenX == 0.0F && screenY == 0.0F) {
        return fallback;
    }

    const float clockwiseFromNorth = std::atan2(screenX, -screenY);
    const int octant = static_cast<int>(std::lround(clockwiseFromNorth / kOctant));
    const int wrapped = ((octant % 8) + 8) % 8;
    return kFacingsByOctant[static_cast<std::size_t>(wrapped)];
}

UnitFacing facingFromDirection(model::Direction direction)
{
    // One grid step per compass direction, with +x East and +y South. Reusing the projection
    // helper keeps the screen mapping in one place.
    constexpr std::array<GridStep, 8> kGridSteps{{{0.0F, -1.0F}, {1.0F, -1.0F}, {1.0F, 0.0F},
        {1.0F, 1.0F}, {0.0F, 1.0F}, {-1.0F, 1.0F}, {-1.0F, 0.0F}, {-1.0F, -1.0F}}};
    const GridStep step = kGridSteps[static_cast<std::size_t>(direction)];
    return facingFromGridMovement(step.dx, step.dy);
}

} // namespace view
