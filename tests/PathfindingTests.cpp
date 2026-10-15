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

TEST(Pathfinding, ReturnsNoRouteWhenTheDestinationIsBlocked)
{
    // Given a grid whose requested destination is blocked.
    const model::GridPos goal{3, 2};
    const auto blocked = [goal](model::GridPos tile) { return tile == goal; };

    // When a unit requests a route to that destination.
    const auto path = model::pathfinding::findPath({0, 0}, goal, blocked);

    // Then no route is returned.
    EXPECT_FALSE(path.has_value());
}

TEST(Pathfinding, FindsOnlyAvailableTilesWithinTheRequestedRadius)
{
    // Given a center tile and one unavailable neighbour.
    const model::GridPos center{0, 0};
    const model::GridPos unavailableTile{1, 0};
    const auto unavailable = [center, unavailableTile](model::GridPos tile) {
        return tile == center || tile == unavailableTile;
    };

    // When three nearby free tiles are requested within one ring.
    const auto tiles = model::pathfinding::nearestFreeTiles(center, 3, unavailable, 1);

    // Then every returned tile is free and no farther than that ring.
    ASSERT_EQ(tiles.size(), 3U);
    for (const model::GridPos tile : tiles) {
        EXPECT_FALSE(unavailable(tile));
        EXPECT_LE(std::max(std::abs(tile.x), std::abs(tile.y)), 1);
    }
}
