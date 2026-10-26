#include "model/time/TimeSystem.hpp"

#include "core/Events.hpp"

namespace model {

TimeSystem::TimeSystem(core::EventBus& bus)
    : bus_(bus)
{
}

void TimeSystem::reset()
{
    clock_.reset();
    publish();
}

void TimeSystem::update(double deltaSeconds)
{
    if (clock_.advance(deltaSeconds) > 0) {
        publish();
    }
}

void TimeSystem::publish() const
{
    bus_.publish(events::TimeChanged{
        clock_.totalTicks(),
        clock_.tickInDay(),
        clock_.day(),
        clock_.week(),
        clock_.month(),
        GameClock::kTicksPerDay,
    });
}

} // namespace model
