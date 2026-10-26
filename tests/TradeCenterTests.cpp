#include "model/build/BuildingType.hpp"
#include "model/economy/Item.hpp"

#include <gtest/gtest.h>

TEST(TradeCenterTests, HasAThreeByThreeFootprint)
{
    EXPECT_EQ(model::buildingSpec(model::BuildingType::TradeCenter).size, 3);
}

TEST(TradeCenterTests, CostsTwoHundredIron)
{
    // The Trade Center has a single cost line: 200 iron.
    const model::BuildingCosts& costs = model::buildingCosts(model::BuildingType::TradeCenter);
    int iron = 0;
    int otherItems = 0;
    for (const model::BuildingCost& cost : costs) {
        if (cost.quantity <= 0 || cost.item == model::ItemId::Count) {
            continue;
        }
        if (cost.item == model::ItemId::Iron) {
            iron += cost.quantity;
        } else {
            otherItems += cost.quantity;
        }
    }
    EXPECT_EQ(iron, 200);
    EXPECT_EQ(otherItems, 0);
}

TEST(TradeCenterTests, TakesTwelveSecondsToBuild)
{
    EXPECT_FLOAT_EQ(model::buildingBuildSeconds(model::BuildingType::TradeCenter), 12.0F);
}

TEST(TradeCenterTests, HasFiveHundredHealth)
{
    EXPECT_EQ(model::buildingMaxHealth(model::BuildingType::TradeCenter), 500);
}
