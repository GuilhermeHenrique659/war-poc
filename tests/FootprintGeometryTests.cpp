#include "model/FootprintGeometry.hpp"

#include <gtest/gtest.h>

namespace {

[[nodiscard]] model::components::Footprint footprint(int x, int y, int size)
{
    return model::components::Footprint{model::GridPos{x, y}, size};
}

} // namespace

TEST(FootprintGeometryTests, ReportsTheCenterTileOfASquareFootprint)
{
    // Given a 2x2 footprint and a 1x1 footprint.
    // When asking for their center tiles.
    // Then both are centered at origin + size / 2 (integer division).
    EXPECT_EQ(model::footprintCenter(footprint(10, 10, 2)), (model::GridPos{11, 11}));
    EXPECT_EQ(model::footprintCenter(footprint(3, 4, 1)), (model::GridPos{3, 4}));
}

TEST(FootprintGeometryTests, BuildsBoundsAndCenterInContinuousCoordinates)
{
    // Given a 3x3 footprint at (1, 2).
    const model::components::Footprint tile = footprint(1, 2, 3);

    // When computing its bounds.
    const model::FootprintBounds bounds = model::footprintBounds(tile);

    // Then the rectangle spans the footprint and its center matches the tile center.
    EXPECT_FLOAT_EQ(bounds.left, 1.0F);
    EXPECT_FLOAT_EQ(bounds.top, 2.0F);
    EXPECT_FLOAT_EQ(bounds.right, 4.0F);
    EXPECT_FLOAT_EQ(bounds.bottom, 5.0F);
    EXPECT_FLOAT_EQ(bounds.centerX(), 2.5F);
    EXPECT_FLOAT_EQ(bounds.centerY(), 3.5F);
}

TEST(FootprintGeometryTests, MeasuresDistanceToTheClosestEdgeOfTheFootprint)
{
    // Given a 2x2 footprint from (10, 10) to (12, 12).
    const model::FootprintBounds bounds = model::footprintBounds(footprint(10, 10, 2));

    // When measuring points inside, beside and diagonally off a corner.
    // Then the distance is zero inside and the nearest-edge distance outside.
    EXPECT_FLOAT_EQ(model::distanceToFootprint(11.0F, 11.0F, bounds), 0.0F);
    EXPECT_FLOAT_EQ(model::distanceToFootprint(7.0F, 11.0F, bounds), 3.0F);
    EXPECT_NEAR(model::distanceToFootprint(6.0F, 9.0F, bounds), 4.1231056F, 1.0e-4F);
}
