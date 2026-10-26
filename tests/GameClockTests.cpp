#include "model/time/GameClock.hpp"

#include <gtest/gtest.h>

TEST(GameClockTests, StartsAtTheFirstDayOfTheFirstWeekAndMonth)
{
    // Given a fresh clock.
    const model::GameClock clock;

    // Then it starts at month 1, week 1, day 1 and no elapsed ticks.
    EXPECT_EQ(clock.totalTicks(), 0U);
    EXPECT_EQ(clock.tickInDay(), 0U);
    EXPECT_EQ(clock.day(), 1U);
    EXPECT_EQ(clock.week(), 1U);
    EXPECT_EQ(clock.month(), 1U);
}

TEST(GameClockTests, AdvancesOneTickPerCompletedSecond)
{
    // Given a fresh clock.
    model::GameClock clock;

    // When half a second passes, no tick is completed.
    EXPECT_EQ(clock.advance(0.5), 0U);
    EXPECT_EQ(clock.totalTicks(), 0U);

    // Then another half second completes exactly one tick.
    EXPECT_EQ(clock.advance(0.5), 1U);
    EXPECT_EQ(clock.totalTicks(), 1U);
    EXPECT_EQ(clock.tickInDay(), 1U);
}

TEST(GameClockTests, KeepsTheSameDayUntilTheLastTick)
{
    // Given a clock advanced to the last tick of the first day.
    model::GameClock clock;

    // When 599 ticks elapse.
    clock.advance(599.0);

    // Then it is still day 1, with one second left.
    EXPECT_EQ(clock.tickInDay(), 599U);
    EXPECT_EQ(clock.day(), 1U);
    EXPECT_EQ(clock.week(), 1U);
    EXPECT_EQ(clock.month(), 1U);
}

TEST(GameClockTests, StartsTheNextDayAfterSixHundredTicks)
{
    // Given a clock advanced through the whole first day.
    model::GameClock clock;

    // When the 600th tick completes.
    clock.advance(600.0);

    // Then the calendar advances to day 2 and the day counter restarts.
    EXPECT_EQ(clock.totalTicks(), 600U);
    EXPECT_EQ(clock.tickInDay(), 0U);
    EXPECT_EQ(clock.day(), 2U);
    EXPECT_EQ(clock.week(), 1U);
    EXPECT_EQ(clock.month(), 1U);
}

TEST(GameClockTests, StartsTheNextWeekAfterSevenDays)
{
    // Given a clock advanced through six complete days and most of the seventh.
    model::GameClock clock;
    clock.advance(7.0 * 600.0 - 1.0);

    // When the last tick of the seventh day completes.
    clock.advance(1.0);

    // Then it is day 8 of week 2.
    EXPECT_EQ(clock.day(), 8U);
    EXPECT_EQ(clock.week(), 2U);
    EXPECT_EQ(clock.month(), 1U);
    EXPECT_EQ(clock.tickInDay(), 0U);
}

TEST(GameClockTests, StartsTheNextMonthAfterFourWeeks)
{
    // Given a clock advanced through 27 complete days and most of the 28th.
    model::GameClock clock;
    clock.advance(28.0 * 600.0 - 1.0);

    // When the last tick of the fourth week completes.
    clock.advance(1.0);

    // Then the calendar restarts month 2 at week 1, day 1.
    EXPECT_EQ(clock.totalTicks(), 28U * 600U);
    EXPECT_EQ(clock.tickInDay(), 0U);
    EXPECT_EQ(clock.day(), 1U);
    EXPECT_EQ(clock.week(), 1U);
    EXPECT_EQ(clock.month(), 2U);
}

TEST(GameClockTests, ProcessesMultipleTicksFromALargeDelta)
{
    // Given a fresh clock.
    model::GameClock clock;

    // When a delta spans two days and three ticks.
    const std::uint64_t ticks = clock.advance(2.0 * 600.0 + 3.0);

    // Then every whole tick is counted and the remainder waits for the next update.
    EXPECT_EQ(ticks, 2U * 600U + 3U);
    EXPECT_EQ(clock.totalTicks(), 2U * 600U + 3U);
    EXPECT_EQ(clock.tickInDay(), 3U);
    EXPECT_EQ(clock.day(), 3U);
}

TEST(GameClockTests, ResetReturnsToTheStart)
{
    // Given a clock advanced past the first day.
    model::GameClock clock;
    clock.advance(601.0);

    // When it is reset.
    clock.reset();

    // Then it starts over at day 1 with no ticks.
    EXPECT_EQ(clock.totalTicks(), 0U);
    EXPECT_EQ(clock.tickInDay(), 0U);
    EXPECT_EQ(clock.day(), 1U);
    EXPECT_EQ(clock.week(), 1U);
    EXPECT_EQ(clock.month(), 1U);
}
