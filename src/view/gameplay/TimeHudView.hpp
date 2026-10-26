#pragma once

#include "core/EventBus.hpp"
#include "core/Events.hpp"

namespace view::gameplay {

// Calendar card shown at the top-right corner during a match.
class TimeHudView {
public:
    explicit TimeHudView(core::EventBus& bus);

    void draw() const;

    [[nodiscard]] bool hasTime() const { return hasTime_; }
    [[nodiscard]] const events::TimeChanged& time() const { return time_; }

private:
    events::TimeChanged time_{};
    bool hasTime_ = false;
};

} // namespace view::gameplay
