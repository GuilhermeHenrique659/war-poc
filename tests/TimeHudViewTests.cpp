#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "view/gameplay/TimeHudView.hpp"

#include <gtest/gtest.h>
#include <raylib.h>

// The test never draws, but the compiled view references raylib's screen size.
int GetScreenWidth()
{
    return 1280;
}

TEST(TimeHudViewTests, StartsHiddenUntilTheFirstTimeNotification)
{
    // Given a view connected to the bus.
    core::EventBus bus;
    const view::gameplay::TimeHudView view{bus};

    // Then there is nothing to show before the match publishes its clock.
    EXPECT_FALSE(view.hasTime());
}

TEST(TimeHudViewTests, StoresTheLatestCalendarState)
{
    // Given a view connected to the bus.
    core::EventBus bus;
    view::gameplay::TimeHudView view{bus};

    // When a calendar update arrives.
    events::TimeChanged time{};
    time.totalTicks = 720;
    time.tickInDay = 120;
    time.day = 2;
    time.week = 1;
    time.month = 1;
    time.ticksPerDay = 600;
    bus.publish(time);

    // Then the view keeps the notification for drawing.
    ASSERT_TRUE(view.hasTime());
    EXPECT_EQ(view.time().totalTicks, 720U);
    EXPECT_EQ(view.time().tickInDay, 120U);
    EXPECT_EQ(view.time().day, 2U);
    EXPECT_EQ(view.time().week, 1U);
    EXPECT_EQ(view.time().month, 1U);
}

TEST(TimeHudViewTests, HidesWhenTheWorldResets)
{
    // Given a view already showing a calendar.
    core::EventBus bus;
    view::gameplay::TimeHudView view{bus};
    bus.publish(events::TimeChanged{});
    ASSERT_TRUE(view.hasTime());

    // When the world is reset.
    bus.publish(events::WorldReset{});

    // Then the old calendar is discarded.
    EXPECT_FALSE(view.hasTime());
}
