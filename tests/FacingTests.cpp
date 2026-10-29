#include "model/Direction.hpp"

#include <gtest/gtest.h>

namespace model {
namespace {

TEST(DirectionTests, MapsEachGridStepToTheCompassDirectionItMovesTowards)
{
    // Given the eight directions a grid movement can take.
    // When the movement delta is converted to a compass direction.
    // Then the result follows the grid axes (+x East, +y South).
    EXPECT_EQ(directionFromGridMovement(1.0F, 0.0F, Direction::North), Direction::East);
    EXPECT_EQ(directionFromGridMovement(0.0F, 1.0F, Direction::North), Direction::South);
    EXPECT_EQ(directionFromGridMovement(1.0F, 1.0F, Direction::North), Direction::SouthEast);
    EXPECT_EQ(directionFromGridMovement(-1.0F, 1.0F, Direction::North), Direction::SouthWest);
    EXPECT_EQ(directionFromGridMovement(-1.0F, 0.0F, Direction::North), Direction::West);
    EXPECT_EQ(directionFromGridMovement(-1.0F, -1.0F, Direction::North), Direction::NorthWest);
    EXPECT_EQ(directionFromGridMovement(0.0F, -1.0F, Direction::North), Direction::North);
    EXPECT_EQ(directionFromGridMovement(1.0F, -1.0F, Direction::North), Direction::NorthEast);
}

TEST(DirectionTests, KeepsTheCurrentDirectionWhenThereIsNoMovement)
{
    // Given a unit that already faces east.
    // When no movement delta is observed.
    // Then the previous direction is preserved.
    EXPECT_EQ(directionFromGridMovement(0.0F, 0.0F, Direction::East), Direction::East);
}

} // namespace
} // namespace model
