#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/time/TimeSystem.hpp"

#include <gtest/gtest.h>

#include <vector>

namespace {

class TimeSystemTests : public ::testing::Test {
protected:
    void SetUp() override
    {
        bus_.subscribe<events::TimeChanged>(
            [this](const events::TimeChanged& event) { published_.push_back(event); });
        bus_.subscribe<events::WeekStarted>(
            [this](const events::WeekStarted& event) { weeks_.push_back(event); });
    }

    core::EventBus bus_;
    std::vector<events::TimeChanged> published_;
    std::vector<events::WeekStarted> weeks_;
};

} // namespace

TEST_F(TimeSystemTests, ResetPublishesTheInitialCalendarState)
{
    // Given a time system.
    model::TimeSystem time{bus_};

    // When the match clock is reset.
    time.reset();

    // Then the initial state is published immediately.
    ASSERT_EQ(published_.size(), 1U);
    EXPECT_EQ(published_[0].totalTicks, 0U);
    EXPECT_EQ(published_[0].tickInDay, 0U);
    EXPECT_EQ(published_[0].day, 1U);
    EXPECT_EQ(published_[0].week, 1U);
    EXPECT_EQ(published_[0].month, 1U);
    EXPECT_EQ(published_[0].ticksPerDay, model::GameClock::kTicksPerDay);
}

TEST_F(TimeSystemTests, DoesNotPublishBeforeAFullTickCompletes)
{
    // Given a running clock.
    model::TimeSystem time{bus_};
    time.reset();
    published_.clear();

    // When less than one second passes.
    time.update(0.9);

    // Then no calendar notification is emitted.
    EXPECT_TRUE(published_.empty());
}

TEST_F(TimeSystemTests, PublishesTheStateAfterEachCompletedTick)
{
    // Given a running clock.
    model::TimeSystem time{bus_};
    time.reset();
    published_.clear();

    // When one second elapses in two updates.
    time.update(0.5);
    time.update(0.5);

    // Then one tick is published.
    ASSERT_EQ(published_.size(), 1U);
    EXPECT_EQ(published_[0].totalTicks, 1U);
    EXPECT_EQ(published_[0].tickInDay, 1U);
    EXPECT_EQ(published_[0].day, 1U);
}

TEST_F(TimeSystemTests, PublishesTheDayRollover)
{
    // Given a running clock.
    model::TimeSystem time{bus_};
    time.reset();
    published_.clear();

    // When a whole day elapses at once.
    time.update(600.0);

    // Then the new day is published once.
    ASSERT_EQ(published_.size(), 1U);
    EXPECT_EQ(published_[0].totalTicks, 600U);
    EXPECT_EQ(published_[0].tickInDay, 0U);
    EXPECT_EQ(published_[0].day, 2U);
    EXPECT_EQ(published_[0].week, 1U);
    EXPECT_EQ(published_[0].month, 1U);
}

TEST_F(TimeSystemTests, PublishesWeekStartedWhenTheFirstWeekCompletes)
{
    // Given a fresh clock.
    model::TimeSystem time{bus_};
    time.reset();
    weeks_.clear();

    // When exactly seven days elapse.
    time.update(static_cast<double>(model::GameClock::kTicksPerDay * model::GameClock::kDaysPerWeek));

    // Then the second week is announced.
    ASSERT_EQ(weeks_.size(), 1U);
    EXPECT_EQ(weeks_[0].week, 2U);
}

TEST_F(TimeSystemTests, DoesNotPublishWeekStartedBeforeAFullWeek)
{
    // Given a fresh clock.
    model::TimeSystem time{bus_};
    time.reset();
    weeks_.clear();

    // When one second short of a week elapses.
    time.update(static_cast<double>(model::GameClock::kTicksPerDay * model::GameClock::kDaysPerWeek) - 1.0);

    // Then no week is announced.
    EXPECT_TRUE(weeks_.empty());
}

TEST_F(TimeSystemTests, PublishesOneWeekStartedPerWeekCrossedInALargeJump)
{
    // Given a fresh clock.
    model::TimeSystem time{bus_};
    time.reset();
    weeks_.clear();

    // When three whole weeks elapse in a single update.
    time.update(static_cast<double>(3 * model::GameClock::kTicksPerDay * model::GameClock::kDaysPerWeek));

    // Then each crossed week is announced once, in order.
    ASSERT_EQ(weeks_.size(), 3U);
    EXPECT_EQ(weeks_[0].week, 2U);
    EXPECT_EQ(weeks_[1].week, 3U);
    EXPECT_EQ(weeks_[2].week, 4U);
}

TEST_F(TimeSystemTests, WeekCounterKeepsGrowingAcrossMonths)
{
    // Given a fresh clock.
    model::TimeSystem time{bus_};
    time.reset();
    weeks_.clear();

    // When a whole month (four weeks) elapses.
    time.update(static_cast<double>(
        model::GameClock::kDaysPerMonth * model::GameClock::kTicksPerDay));

    // Then the week counter is absolute instead of restarting with the month.
    ASSERT_EQ(weeks_.size(), 4U);
    EXPECT_EQ(weeks_.back().week, 5U);
}
