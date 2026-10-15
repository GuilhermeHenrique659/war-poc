#include "core/EventBus.hpp"

#include <gtest/gtest.h>

namespace {

struct ValueChanged {
    int value;
};

} // namespace

TEST(EventBus, DefersSubscriptionsAddedDuringTheCurrentDispatch)
{
    // Given a handler that subscribes another handler while receiving an event.
    core::EventBus bus;
    int originalHandlerCalls = 0;
    int addedHandlerCalls = 0;
    bus.subscribe<ValueChanged>([&](const ValueChanged&) {
        ++originalHandlerCalls;
        bus.subscribe<ValueChanged>([&](const ValueChanged&) { ++addedHandlerCalls; });
    });

    // When the event is published for the first time.
    bus.publish(ValueChanged{42});

    // Then only the handler that existed at the start of dispatch runs.
    EXPECT_EQ(originalHandlerCalls, 1);
    EXPECT_EQ(addedHandlerCalls, 0);

    // When the event is published again.
    bus.publish(ValueChanged{42});

    // Then the previous subscription receives the next event.
    EXPECT_EQ(originalHandlerCalls, 2);
    EXPECT_EQ(addedHandlerCalls, 1);
}
