#include "model/Direction.hpp"
#include "raylib_win.h"
#include "view/units/UnitFacing.hpp"

#include <gtest/gtest.h>

namespace view {
namespace {

TEST(UnitFacingTests, MapsEveryIsometricGridStepToTheScreenFacingItAppearsToMove)
{
    // Given the eight directions a unit path can take in grid space.
    // When the movement is projected to the isometric screen.
    // Then the facing matches the direction the unit visually walks.
    EXPECT_EQ(facingFromGridMovement(1.0F, 0.0F, UnitFacing::North), UnitFacing::SouthEast);
    EXPECT_EQ(facingFromGridMovement(0.0F, 1.0F, UnitFacing::North), UnitFacing::SouthWest);
    EXPECT_EQ(facingFromGridMovement(1.0F, 1.0F, UnitFacing::North), UnitFacing::South);
    EXPECT_EQ(facingFromGridMovement(-1.0F, 1.0F, UnitFacing::North), UnitFacing::West);
    EXPECT_EQ(facingFromGridMovement(-1.0F, 0.0F, UnitFacing::North), UnitFacing::NorthWest);
    EXPECT_EQ(facingFromGridMovement(-1.0F, -1.0F, UnitFacing::North), UnitFacing::North);
    EXPECT_EQ(facingFromGridMovement(0.0F, -1.0F, UnitFacing::North), UnitFacing::NorthEast);
    EXPECT_EQ(facingFromGridMovement(1.0F, -1.0F, UnitFacing::North), UnitFacing::East);
}

TEST(UnitFacingTests, KeepsTheCurrentFacingWhenTheUnitDidNotMove)
{
    // Given a unit that already faces east.
    // When no movement delta is observed.
    // Then the previous facing is preserved.
    EXPECT_EQ(facingFromGridMovement(0.0F, 0.0F, UnitFacing::East), UnitFacing::East);
}

TEST(UnitFacingTests, MapsEachModelDirectionToTheScreenFacingItAppearsToWalk)
{
    // Given the eight grid directions stored by the model.
    // When each one is projected to the isometric screen.
    // Then the sprite facing matches the direction the unit visually walks.
    EXPECT_EQ(facingFromDirection(model::Direction::North), UnitFacing::NorthEast);
    EXPECT_EQ(facingFromDirection(model::Direction::NorthEast), UnitFacing::East);
    EXPECT_EQ(facingFromDirection(model::Direction::East), UnitFacing::SouthEast);
    EXPECT_EQ(facingFromDirection(model::Direction::SouthEast), UnitFacing::South);
    EXPECT_EQ(facingFromDirection(model::Direction::South), UnitFacing::SouthWest);
    EXPECT_EQ(facingFromDirection(model::Direction::SouthWest), UnitFacing::West);
    EXPECT_EQ(facingFromDirection(model::Direction::West), UnitFacing::NorthWest);
    EXPECT_EQ(facingFromDirection(model::Direction::NorthWest), UnitFacing::North);
}

} // namespace
} // namespace view
