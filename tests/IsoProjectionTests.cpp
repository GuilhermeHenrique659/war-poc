#include "view/IsoProjection.hpp"

#include <gtest/gtest.h>

namespace view::iso {
namespace {

TEST(IsoProjectionTests, PlacesSpriteFromNorthWithOptionalOverlap)
{
    const Rectangle destination
        = spriteDestinationFromNorth(3.0F, 5.0F, Vector2{128.0F, 64.0F}, 1.0F, 2.0F);

    EXPECT_FLOAT_EQ(destination.x, -193.0F);
    EXPECT_FLOAT_EQ(destination.y, 254.0F);
    EXPECT_FLOAT_EQ(destination.width, 130.0F);
    EXPECT_FLOAT_EQ(destination.height, 68.0F);
}

TEST(IsoProjectionTests, PlacesSpriteFromSouthUsingFootprintAndFrameAspectRatio)
{
    const Rectangle destination
        = spriteDestinationFromSouth(2.0F, 4.0F, Vector2{2.0F, 2.0F}, Vector2{443.5F, 443.5F});

    EXPECT_FLOAT_EQ(destination.x, -256.0F);
    EXPECT_FLOAT_EQ(destination.y, 64.0F);
    EXPECT_FLOAT_EQ(destination.width, 256.0F);
    EXPECT_FLOAT_EQ(destination.height, 256.0F);
}

} // namespace
} // namespace view::iso
