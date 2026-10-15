#include "model/Pathfinding.hpp"

#include <gtest/gtest.h>

#include <cstdlib>
#include <unordered_set>

TEST(Pathfinding, ReturnsTheDirectDiagonalRouteOnAnOpenGrid)
{
    // Given an open grid.
    const auto blocked = [](model::GridPos) { return false; };

    // When a unit moves to a diagonally adjacent tile.
    const auto path = model::pathfinding::findPath({0, 0}, {1, 1}, blocked);

    // Then the route contains only the destination.
    const model::GridPos expectedGoal{1, 1};
    ASSERT_TRUE(path.has_value());
    ASSERT_EQ(path->size(), 1U);
    EXPECT_TRUE(path->front() == expectedGoal);
}

TEST(Pathfinding, RoutesAroundABlockedDiagonalCorner)
{
    // Given both orthogonal tiles next to a diagonal move are blocked.
    const std::unordered_set<model::GridPos, model::GridPosHash> blockedTiles{
        model::GridPos{1, 0}, model::GridPos{0, 1}};
    const auto blocked = [&blockedTiles](model::GridPos tile) { return blockedTiles.contains(tile); };

    // When a unit requests the diagonal destination.
    const model::GridPos start{0, 0};
    const model::GridPos goal{1, 1};
    const auto path = model::pathfinding::findPath(start, goal, blocked);

    // Then the route reaches it without making a corner-cutting step.
    ASSERT_TRUE(path.has_value());
    ASSERT_GT(path->size(), 1U);
    EXPECT_TRUE(path->back() == goal);

    model::GridPos previous = start;
    for (const model::GridPos tile : *path) {
        const int deltaX = tile.x - previous.x;
        const int deltaY = tile.y - previous.y;
        EXPECT_LE(std::abs(deltaX), 1);
        EXPECT_LE(std::abs(deltaY), 1);
        EXPECT_TRUE(deltaX != 0 || deltaY != 0);
        if (std::abs(deltaX) == 1 && std::abs(deltaY) == 1) {
            const model::GridPos horizontal{previous.x + deltaX, previous.y};
            const model::GridPos vertical{previous.x, previous.y + deltaY};
            EXPECT_FALSE(blocked(horizontal));
            EXPECT_FALSE(blocked(vertical));
        }
        previous = tile;
    }
}
