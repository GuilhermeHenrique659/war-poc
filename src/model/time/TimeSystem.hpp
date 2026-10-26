#pragma once

#include "core/EventBus.hpp"
#include "model/time/GameClock.hpp"

namespace model {

// Owns the match calendar and publishes its state whenever a whole tick completes.
class TimeSystem {
public:
    explicit TimeSystem(core::EventBus& bus);

    // Restarts the calendar and publishes the initial state.
    void reset();

    // Advances by elapsed gameplay seconds and publishes the new state after whole ticks.
    void update(double deltaSeconds);

    [[nodiscard]] const GameClock& clock() const { return clock_; }

private:
    void publish() const;
    void publishWeeks();
    [[nodiscard]] std::uint32_t absoluteWeek() const;

    core::EventBus& bus_;
    GameClock clock_;
    std::uint32_t lastWeek_ = 1;
};

} // namespace model
