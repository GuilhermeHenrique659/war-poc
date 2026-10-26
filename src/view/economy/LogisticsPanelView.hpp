#pragma once

#include "core/EventBus.hpp"
#include "core/Events.hpp"

namespace view::gameplay {

class LogisticsPanelView {
public:
    explicit LogisticsPanelView(core::EventBus& bus);

    void draw() const;

private:
    events::LogisticsPanelChanged panel_;
    model::PlayerId localPlayerId_ = model::kNoPlayer;
};

} // namespace view::gameplay
