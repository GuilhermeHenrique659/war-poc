#include "model/time/TimeSystem.hpp"

#include "core/Events.hpp"

#include <cstdint>

namespace model {

TimeSystem::TimeSystem(core::EventBus& bus)
    : bus_(bus)
{
}

void TimeSystem::reset()
{
    clock_.reset();
    lastWeek_ = 1;
    publish();
}

void TimeSystem::update(double deltaSeconds)
{
    if (clock_.advance(deltaSeconds) == 0) {
        return;
    }
    publishWeeks();
    publish();
}
std::uint32_t TimeSystem::absoluteWeek() const
{
    constexpr std::uint64_t kTicksPerWeek =
        static_cast<std::uint64_t>(GameClock::kTicksPerDay) * GameClock::kDaysPerWeek;
    return static_cast<std::uint32_t>(clock_.totalTicks() / kTicksPerWeek) + 1;
}

void TimeSystem::publishWeeks()
{
    const std::uint32_t week = absoluteWeek();
    while (lastWeek_ < week) {
        ++lastWeek_;
        bus_.publish(events::WeekStarted{lastWeek_});
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
