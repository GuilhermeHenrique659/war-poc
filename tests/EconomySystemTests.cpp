#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/economy/EconomySystem.hpp"
#include "model/economy/Item.hpp"

#include <gtest/gtest.h>

#include <vector>

namespace {

constexpr model::PlayerId kPlayer = 7;

class EconomySystemTests : public ::testing::Test {
protected:
    EconomySystemTests()
        : economy_(bus_)
    {
        bus_.subscribe<events::CreditChanged>(
            [this](const events::CreditChanged& event) { credits_.push_back(event); });
        bus_.subscribe<events::MarketPricesChanged>(
            [this](const events::MarketPricesChanged& event) { prices_.push_back(event); });
        bus_.subscribe<events::TradeCompleted>(
            [this](const events::TradeCompleted& event) { trades_.push_back(event); });
    }

    core::EventBus bus_;
    model::EconomySystem economy_;
    std::vector<events::CreditChanged> credits_;
    std::vector<events::MarketPricesChanged> prices_;
    std::vector<events::TradeCompleted> trades_;
};

} // namespace

TEST_F(EconomySystemTests, StartsANewAccountWithTheConfiguredCredit)
{
    // When a player joins the match.
    economy_.initializePlayer(kPlayer);

    // Then the account opens with the starting credit and is announced.
    EXPECT_DOUBLE_EQ(economy_.balance(kPlayer), model::kStartingCredit);
    ASSERT_EQ(credits_.size(), 1U);
    EXPECT_EQ(credits_[0].player, kPlayer);
    EXPECT_DOUBLE_EQ(credits_[0].balance, model::kStartingCredit);
    EXPECT_DOUBLE_EQ(credits_[0].weeklyInterest, 0.0);
}

TEST_F(EconomySystemTests, ConsumingResourcesChargesTheirMarketPrice)
{
    // Given a player with the starting credit.
    economy_.initializePlayer(kPlayer);

    // When ten iron and two steel are consumed.
    bus_.publish(events::ResourceConsumed{kPlayer, model::ItemId::Iron, 10});
    bus_.publish(events::ResourceConsumed{kPlayer, model::ItemId::Steel, 2});

    // Then every unit costs its base price.
    EXPECT_DOUBLE_EQ(economy_.balance(kPlayer), model::kStartingCredit - 10.0 - 10.0);
    ASSERT_FALSE(credits_.empty());
    EXPECT_DOUBLE_EQ(credits_.back().balance, model::kStartingCredit - 20.0);
}

TEST_F(EconomySystemTests, ProductionIsNotChargedAndLowersTheNextWeeksPrice)
{
    // Given a player with the starting credit.
    economy_.initializePlayer(kPlayer);

    // When the player produces iron.
    bus_.publish(events::ResourceProduced{kPlayer, model::ItemId::Iron, 500});

    // Then no credit was spent, but the supply is recorded for the next week.
    EXPECT_DOUBLE_EQ(economy_.balance(kPlayer), model::kStartingCredit);
    bus_.publish(events::WeekStarted{2});
    EXPECT_LT(economy_.market().price(model::ItemId::Iron),
        economy_.market().basePrice(model::ItemId::Iron));
}

TEST_F(EconomySystemTests, SellingCreditsTheMarketPriceAndCountsAsDemand)
{
    // Given a player with the starting credit.
    economy_.initializePlayer(kPlayer);

    // When two tools are sold at the base price of 25.
    const double revenue = economy_.sell(kPlayer, model::ItemId::Tools, 2);

    // Then the credit grows and the sale is both recorded and announced.
    EXPECT_DOUBLE_EQ(revenue, 50.0);
    EXPECT_DOUBLE_EQ(economy_.balance(kPlayer), model::kStartingCredit + 50.0);
    EXPECT_EQ(economy_.market().flows(model::ItemId::Tools).sold, 2);
    ASSERT_EQ(trades_.size(), 1U);
    EXPECT_EQ(trades_[0].item, model::ItemId::Tools);
    EXPECT_EQ(trades_[0].quantity, 2);
    EXPECT_DOUBLE_EQ(trades_[0].revenue, 50.0);
}

TEST_F(EconomySystemTests, SellingNothingDoesNotChangeTheBalance)
{
    // Given a player with the starting credit.
    economy_.initializePlayer(kPlayer);

    // When an empty or invalid sale is attempted.
    EXPECT_DOUBLE_EQ(economy_.sell(kPlayer, model::ItemId::Iron, 0), 0.0);
    EXPECT_DOUBLE_EQ(economy_.sell(kPlayer, model::ItemId::Iron, -5), 0.0);

    // Then the balance is untouched and nothing was recorded.
    EXPECT_DOUBLE_EQ(economy_.balance(kPlayer), model::kStartingCredit);
    EXPECT_TRUE(trades_.empty());
    EXPECT_EQ(economy_.market().flows(model::ItemId::Iron).sold, 0);
}

TEST_F(EconomySystemTests, NegativeBalancePaysFivePercentRoundedUpEveryWeek)
{
    // Given a debt of 110 credited by consuming 1110 iron.
    economy_.initializePlayer(kPlayer);
    bus_.publish(events::ResourceConsumed{kPlayer, model::ItemId::Iron, 1110});
    ASSERT_DOUBLE_EQ(economy_.balance(kPlayer), -110.0);
    EXPECT_DOUBLE_EQ(economy_.weeklyInterest(kPlayer), 6.0);

    // When the week closes.
    bus_.publish(events::WeekStarted{2});

    // Then the rounded-up interest is added to the debt.
    EXPECT_DOUBLE_EQ(economy_.balance(kPlayer), -116.0);
    EXPECT_DOUBLE_EQ(economy_.weeklyInterest(kPlayer), 6.0);
}

TEST_F(EconomySystemTests, PositiveBalanceEarnsNoInterest)
{
    // Given a player with the starting credit.
    economy_.initializePlayer(kPlayer);

    // When a week closes.
    bus_.publish(events::WeekStarted{2});

    // Then the balance is unchanged.
    EXPECT_DOUBLE_EQ(economy_.balance(kPlayer), model::kStartingCredit);
    EXPECT_DOUBLE_EQ(economy_.weeklyInterest(kPlayer), 0.0);
}

TEST_F(EconomySystemTests, InterestKeepsCompoundingOnTheAccumulatedDebt)
{
    // Given a debt of 110.
    economy_.initializePlayer(kPlayer);
    bus_.publish(events::ResourceConsumed{kPlayer, model::ItemId::Iron, 1110});

    // When two weeks close.
    bus_.publish(events::WeekStarted{2});
    bus_.publish(events::WeekStarted{3});

    // Then the second interest is computed over the already increased debt.
    EXPECT_DOUBLE_EQ(economy_.balance(kPlayer), -122.0);
}

TEST_F(EconomySystemTests, ClosingTheWeekPublishesTheNewPrices)
{
    // Given a player and some demand recorded during the week.
    economy_.initializePlayer(kPlayer);
    bus_.publish(events::ResourceConsumed{kPlayer, model::ItemId::Iron, 500});

    // When the week closes.
    bus_.publish(events::WeekStarted{2});

    // Then the updated price of every item is announced.
    ASSERT_EQ(prices_.size(), 1U);
    EXPECT_DOUBLE_EQ(prices_[0].prices[model::itemIndex(model::ItemId::Iron)],
        economy_.market().price(model::ItemId::Iron));
    EXPECT_GT(prices_[0].prices[model::itemIndex(model::ItemId::Iron)],
        economy_.market().basePrice(model::ItemId::Iron));
}

TEST_F(EconomySystemTests, DebugModeSkipsCreditCharges)
{
    // Given a player with debug mode enabled.
    economy_.initializePlayer(kPlayer);
    bus_.publish(events::DebugToggleRequested{});

    // When resources are consumed.
    bus_.publish(events::ResourceConsumed{kPlayer, model::ItemId::Iron, 10});

    // Then nothing is charged.
    EXPECT_DOUBLE_EQ(economy_.balance(kPlayer), model::kStartingCredit);

    // When debug mode is disabled and more iron is consumed.
    bus_.publish(events::DebugToggleRequested{});
    bus_.publish(events::ResourceConsumed{kPlayer, model::ItemId::Iron, 10});

    // Then charges resume.
    EXPECT_DOUBLE_EQ(economy_.balance(kPlayer), model::kStartingCredit - 10.0);
}

TEST_F(EconomySystemTests, ResetClearsAccountsAndMarket)
{
    // Given a player with a debt and a moved market.
    economy_.initializePlayer(kPlayer);
    bus_.publish(events::ResourceConsumed{kPlayer, model::ItemId::Iron, 1500});
    bus_.publish(events::WeekStarted{2});

    // When the match is reset.
    economy_.reset();

    // Then the account is gone and the prices are back at their base.
    EXPECT_DOUBLE_EQ(economy_.balance(kPlayer), 0.0);
    EXPECT_DOUBLE_EQ(economy_.market().price(model::ItemId::Iron),
        economy_.market().basePrice(model::ItemId::Iron));
}
