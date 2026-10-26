#pragma once

#include <cstdint>

namespace model {

// Gameplay calendar: one tick per second, 600 ticks per day, 7 days per week
// and 4 weeks per month.
class GameClock {
public:
    static constexpr std::uint32_t kTicksPerDay = 600;
    static constexpr std::uint32_t kDaysPerWeek = 7;
    static constexpr std::uint32_t kWeeksPerMonth = 4;
    static constexpr std::uint32_t kDaysPerMonth = kDaysPerWeek * kWeeksPerMonth;
    static constexpr double kSecondsPerTick = 1.0;

    // Advances by elapsed gameplay seconds and returns how many whole ticks completed.
    std::uint64_t advance(double deltaSeconds);

    void reset();

    [[nodiscard]] std::uint64_t totalTicks() const;
    [[nodiscard]] std::uint32_t tickInDay() const;
    [[nodiscard]] std::uint32_t day() const;
    [[nodiscard]] std::uint32_t week() const;
    [[nodiscard]] std::uint32_t month() const;

private:
    std::uint64_t totalTicks_ = 0;
    double partialSeconds_ = 0.0;
};

} // namespace model
