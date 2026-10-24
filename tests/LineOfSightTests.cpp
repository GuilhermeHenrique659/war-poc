#include "model/combat/LineOfSight.hpp"

#include <gtest/gtest.h>

#include <initializer_list>

namespace {

[[nodiscard]] model::combat::TileBlocker blockedAt(std::initializer_list<model::GridPos> tiles)
{
    return [tiles](model::GridPos tile) {
        for (const model::GridPos& blocked : tiles) {
            if (tile == blocked) {
                return true;
            }
        }
        return false;
    };
}

} // namespace

TEST(LineOfSightTests, ClearStraightLineIsVisible)
{
    // Given an empty corridor between two tiles.
    // When the line of sight is traced.
    // Then nothing blocks it.
    EXPECT_TRUE(model::combat::hasLineOfSight({0, 0}, {4, 0}, blockedAt({})));
}

TEST(LineOfSightTests, TileBetweenShooterAndTargetBlocksTheLine)
{
    // Given a blocker two tiles to the east.
    // When the line crosses it.
    // Then the shot is blocked.
    EXPECT_FALSE(model::combat::hasLineOfSight({0, 0}, {4, 0}, blockedAt({{2, 0}})));
}

TEST(LineOfSightTests, EndpointTilesAreIgnored)
{
    // Given blockers only on the shooter and target tiles.
    // When the line of sight is traced.
    // Then the endpoints never block their own shot.
    EXPECT_TRUE(model::combat::hasLineOfSight({0, 0}, {1, 0}, blockedAt({{0, 0}, {1, 0}})));
}

TEST(LineOfSightTests, DiagonalObstacleBlocksTheLine)
{
    // Given a blocker on the diagonal path.
    // When the line of sight is traced.
    // Then the shot is blocked.
    EXPECT_FALSE(model::combat::hasLineOfSight({0, 0}, {3, 3}, blockedAt({{2, 2}})));
}

TEST(LineOfSightTests, TileOffTheLineDoesNotBlock)
{
    // Given a blocker one tile beside the straight path.
    // When the line of sight is traced.
    // Then the shot is clear.
    EXPECT_TRUE(model::combat::hasLineOfSight({0, 0}, {4, 0}, blockedAt({{2, 1}})));
}

TEST(LineOfSightTests, SameTileIsAlwaysVisible)
{
    // Given a shooter and a target sharing a tile.
    // When the line of sight is traced.
    // Then it is trivially clear.
    EXPECT_TRUE(model::combat::hasLineOfSight({3, 3}, {3, 3}, blockedAt({{3, 3}})));
}
