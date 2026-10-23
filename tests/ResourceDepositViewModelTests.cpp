#include "raylib_win.h"
#include "view/world/ResourceDepositViewModel.hpp"

#include <gtest/gtest.h>

namespace view::gameplay {
namespace {

TEST(ResourceDepositViewModelTests, PlacesEachResourceTileOnItsIsometricGridCell)
{
    const model::GridPos tile{3, 5};

    const Rectangle destination = ResourceDepositViewModel::tileDestination(tile);

    EXPECT_FLOAT_EQ(destination.x, -192.0F);
    EXPECT_FLOAT_EQ(destination.y, 256.0F);
    EXPECT_FLOAT_EQ(destination.width, 128.0F);
    EXPECT_FLOAT_EQ(destination.height, 64.0F);
}

TEST(ResourceDepositViewModelTests, SelectsTextureForEachMineralDeposit)
{
    EXPECT_STREQ(ResourceDepositViewModel::textureKey(model::ItemId::Iron), "deposit/iron");
    EXPECT_STREQ(ResourceDepositViewModel::textureKey(model::ItemId::Coal), "deposit/coal");
    EXPECT_STREQ(ResourceDepositViewModel::textureKey(model::ItemId::Copper), "deposit/copper");
    EXPECT_EQ(ResourceDepositViewModel::textureKey(model::ItemId::Steel), nullptr);
}

} // namespace
} // namespace view::gameplay
