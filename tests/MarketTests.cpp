#include "model/economy/Item.hpp"
#include "model/economy/Market.hpp"

#include <gtest/gtest.h>

namespace {

constexpr model::ItemId kIron = model::ItemId::Iron;
constexpr model::ItemId kSteel = model::ItemId::Steel;

} // namespace

TEST(MarketTests, StartsEveryItemAtItsBasePrice)
{
    // Given a fresh market.
    model::Market market;

    // Then every item trades at its base price.
    EXPECT_DOUBLE_EQ(market.price(kIron), market.basePrice(kIron));
    EXPECT_DOUBLE_EQ(market.price(model::ItemId::Coal), market.basePrice(model::ItemId::Coal));
    EXPECT_DOUBLE_EQ(market.price(kSteel), market.basePrice(kSteel));
    EXPECT_DOUBLE_EQ(market.price(model::ItemId::Copper), market.basePrice(model::ItemId::Copper));
    EXPECT_DOUBLE_EQ(market.price(model::ItemId::Electronics), market.basePrice(model::ItemId::Electronics));
    EXPECT_DOUBLE_EQ(market.price(model::ItemId::Tools), market.basePrice(model::ItemId::Tools));
    EXPECT_DOUBLE_EQ(market.price(model::ItemId::Weapon), market.basePrice(model::ItemId::Weapon));
    EXPECT_DOUBLE_EQ(market.price(model::ItemId::Ammo), market.basePrice(model::ItemId::Ammo));
}

TEST(MarketTests, InternalConsumptionRaisesTheNextWeeksPrice)
{
    // Given 250 iron consumed internally this week.
    model::Market market;
    market.recordConsumption(kIron, 250);

    // When the week closes.
    market.closeWeek();

    // Then the next week's price reflects the added demand.
    EXPECT_GT(market.price(kIron), market.basePrice(kIron));
}

TEST(MarketTests, ProductionLowersTheNextWeeksPrice)
{
    // Given 500 iron produced with no demand.
    model::Market market;
    market.recordProduction(kIron, 500);

    // When the week closes.
    market.closeWeek();

    // Then abundance lowers the price.
    EXPECT_LT(market.price(kIron), market.basePrice(kIron));
}

TEST(MarketTests, SalesToTheTradeCenterAlsoCountAsDemand)
{
    // Given 250 iron sold externally this week.
    model::Market market;
    market.recordSale(kIron, 250);

    // When the week closes.
    market.closeWeek();

    // Then the external demand raises the price.
    EXPECT_GT(market.price(kIron), market.basePrice(kIron));
}

TEST(MarketTests, PriceIsClampedBetweenHalfAndTwiceTheBasePrice)
{
    // Given an extreme demand and an extreme supply in different items.
    model::Market market;
    market.recordConsumption(kIron, 1'000'000);
    market.recordProduction(kSteel, 1'000'000);

    // When the week closes.
    market.closeWeek();

    // Then prices stay inside the configured band.
    EXPECT_DOUBLE_EQ(market.price(kIron), market.basePrice(kIron) * 2.0);
    EXPECT_DOUBLE_EQ(market.price(kSteel), market.basePrice(kSteel) * 0.5);
}

TEST(MarketTests, ClosingAWeekWithoutFlowsReturnsToTheBasePrice)
{
    // Given a market that raised iron last week.
    model::Market market;
    market.recordConsumption(kIron, 500);
    market.closeWeek();
    ASSERT_GT(market.price(kIron), market.basePrice(kIron));

    // When a full week passes without any flow.
    market.closeWeek();

    // Then the price is back at its base.
    EXPECT_DOUBLE_EQ(market.price(kIron), market.basePrice(kIron));
}

TEST(MarketTests, ClosingAWeekClearsTheFlowCounters)
{
    // Given a market with recorded flows.
    model::Market market;
    market.recordProduction(kIron, 100);
    market.recordConsumption(kIron, 20);
    market.recordSale(kIron, 30);

    // When the week closes.
    market.closeWeek();

    // Then the counters are ready for the next week.
    EXPECT_EQ(market.flows(kIron).produced, 0);
    EXPECT_EQ(market.flows(kIron).consumed, 0);
    EXPECT_EQ(market.flows(kIron).sold, 0);
}

TEST(MarketTests, IgnoresNonPositiveQuantitiesAndTheSentinelItem)
{
    // Given a market.
    model::Market market;

    // When invalid flows are recorded.
    market.recordProduction(kIron, 0);
    market.recordProduction(kIron, -10);
    market.recordConsumption(model::ItemId::Count, 100);
    market.recordSale(kIron, -1);

    // Then nothing changed.
    EXPECT_EQ(market.flows(kIron).produced, 0);
    EXPECT_EQ(market.flows(kIron).consumed, 0);
    EXPECT_EQ(market.flows(kIron).sold, 0);
}

TEST(MarketTests, PricesOfDifferentItemsAreIndependent)
{
    // Given iron demand only.
    model::Market market;
    market.recordConsumption(kIron, 500);

    // When the week closes.
    market.closeWeek();

    // Then steel stayed at its base price.
    EXPECT_GT(market.price(kIron), market.basePrice(kIron));
    EXPECT_DOUBLE_EQ(market.price(kSteel), market.basePrice(kSteel));
}
