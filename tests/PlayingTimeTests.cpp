#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/PlayingModel.hpp"

#include <gtest/gtest.h>

#include <vector>

TEST(PlayingTimeTests, PublishesTheInitialCalendarWhenTheMatchStarts)
{
    // Given a new match with an observer for the calendar.
    core::EventBus bus;
    model::PlayingModel playing{bus};
    std::vector<events::TimeChanged> published;
    bus.subscribe<events::TimeChanged>(
        [&published](const events::TimeChanged& event) { published.push_back(event); });

    // When the match starts.
    bus.publish(events::StartMatchRequested{"Jogador"});

    // Then the first day is announced immediately.
    ASSERT_FALSE(published.empty());
    EXPECT_EQ(published.back().totalTicks, 0U);
    EXPECT_EQ(published.back().day, 1U);
    EXPECT_EQ(published.back().week, 1U);
    EXPECT_EQ(published.back().month, 1U);
}

TEST(PlayingTimeTests, AdvancesOnlyWhileTheMatchIsRunning)
{
    // Given a running match.
    core::EventBus bus;
    model::PlayingModel playing{bus};
    std::vector<events::TimeChanged> published;
    bus.subscribe<events::TimeChanged>(
        [&published](const events::TimeChanged& event) { published.push_back(event); });
    bus.publish(events::StartMatchRequested{"Jogador"});
    published.clear();

    // When less than one second passes, nothing changes.
    playing.update(0.5F);
    EXPECT_TRUE(published.empty());

    // Then a full second publishes one tick.
    playing.update(0.5F);
    ASSERT_EQ(published.size(), 1U);
    EXPECT_EQ(published[0].totalTicks, 1U);

    // When the match is abandoned, the clock stops.
    bus.publish(events::ReturnToMenuRequested{});
    published.clear();
    playing.update(5.0F);
    EXPECT_TRUE(published.empty());
}

TEST(PlayingTimeTests, StartsTheNextMatchAtTheFirstDayAgain)
{
    // Given a match that advanced past the first day.
    core::EventBus bus;
    model::PlayingModel playing{bus};
    std::vector<events::TimeChanged> published;
    bus.subscribe<events::TimeChanged>(
        [&published](const events::TimeChanged& event) { published.push_back(event); });
    bus.publish(events::StartMatchRequested{"Jogador"});
    playing.update(601.0F);
    ASSERT_FALSE(published.empty());
    EXPECT_EQ(published.back().day, 2U);

    // When a new match starts.
    bus.publish(events::StartMatchRequested{"Jogador"});
    playing.update(1.0F);

    // Then the calendar restarts at day 1 and counts the new match's first tick.
    ASSERT_FALSE(published.empty());
    EXPECT_EQ(published.back().totalTicks, 1U);
    EXPECT_EQ(published.back().day, 1U);
    EXPECT_EQ(published.back().week, 1U);
    EXPECT_EQ(published.back().month, 1U);
}
