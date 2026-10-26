#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/PlayingModel.hpp"
#include "model/economy/EconomySystem.hpp"
#include "model/economy/Item.hpp"
#include "model/player/Player.hpp"

#include <gtest/gtest.h>

#include <vector>

TEST(PlayingEconomyTests, StartsTheMatchWithTheConfiguredCredit)
{
    // Given a new match and an observer for the credit account.
    core::EventBus bus;
    model::PlayingModel playing{bus};
    model::PlayerId localPlayer = model::kNoPlayer;
    std::vector<events::CreditChanged> credits;
    bus.subscribe<events::LocalPlayerChanged>(
        [&localPlayer](const events::LocalPlayerChanged& event) { localPlayer = event.player; });
    bus.subscribe<events::CreditChanged>(
        [&credits](const events::CreditChanged& event) { credits.push_back(event); });

    // When the match starts.
    bus.publish(events::StartMatchRequested{"Jogador"});

    // Then the local player receives the starting credit.
    ASSERT_NE(localPlayer, model::kNoPlayer);
    ASSERT_FALSE(credits.empty());
    EXPECT_EQ(credits.back().player, localPlayer);
    EXPECT_DOUBLE_EQ(credits.back().balance, model::kStartingCredit);
}

TEST(PlayingEconomyTests, ChargesWeeklyInterestOnADeficitBuiltDuringTheMatch)
{
    // Given a running match whose player consumed more iron than the credit covers.
    core::EventBus bus;
    model::PlayingModel playing{bus};
    model::PlayerId localPlayer = model::kNoPlayer;
    std::vector<events::CreditChanged> credits;
    bus.subscribe<events::LocalPlayerChanged>(
        [&localPlayer](const events::LocalPlayerChanged& event) { localPlayer = event.player; });
    bus.subscribe<events::CreditChanged>(
        [&credits](const events::CreditChanged& event) { credits.push_back(event); });
    bus.publish(events::StartMatchRequested{"Jogador"});
    bus.publish(events::ResourceConsumed{localPlayer, model::ItemId::Iron, 2000});
    credits.clear();

    // When a whole week passes.
    playing.update(4200.0F);

    // Then the 5% interest is charged over the accumulated debt.
    ASSERT_FALSE(credits.empty());
    EXPECT_EQ(credits.back().player, localPlayer);
    EXPECT_DOUBLE_EQ(credits.back().balance, -1050.0);
}
