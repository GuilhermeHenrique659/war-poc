#include "model/time/GameClock.hpp"

namespace model {

namespace {

constexpr double kTickEpsilon = 1e-9;

} // namespace

std::uint64_t GameClock::advance(double deltaSeconds)
{
    if (deltaSeconds <= 0.0) {
        return 0;
    }

    partialSeconds_ += deltaSeconds;
    const auto completed = static_cast<std::uint64_t>((partialSeconds_ + kTickEpsilon) / kSecondsPerTick);
    if (completed == 0) {
        return 0;
    }

    partialSeconds_ -= static_cast<double>(completed) * kSecondsPerTick;
    totalTicks_ += completed;
    return completed;
}

void GameClock::reset()
{
    totalTicks_ = 0;
    partialSeconds_ = 0.0;
}

std::uint64_t GameClock::totalTicks() const
{
    return totalTicks_;
}

std::uint32_t GameClock::tickInDay() const
{
    return static_cast<std::uint32_t>(totalTicks_ % kTicksPerDay);
}

std::uint32_t GameClock::day() const
{
    const std::uint64_t elapsedDays = totalTicks_ / kTicksPerDay;
    return static_cast<std::uint32_t>(elapsedDays % kDaysPerMonth) + 1;
}

std::uint32_t GameClock::week() const
{
    const std::uint64_t elapsedDays = totalTicks_ / kTicksPerDay;
    return static_cast<std::uint32_t>((elapsedDays % kDaysPerMonth) / kDaysPerWeek) + 1;
}

std::uint32_t GameClock::month() const
{
    const std::uint64_t elapsedDays = totalTicks_ / kTicksPerDay;
    return static_cast<std::uint32_t>(elapsedDays / kDaysPerMonth) + 1;
}

} // namespace model
