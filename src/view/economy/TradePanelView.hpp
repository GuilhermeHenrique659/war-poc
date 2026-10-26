#pragma once

#include "core/EventBus.hpp"
#include "core/Events.hpp"

namespace view::gameplay {

// Read-only panel of the selected Trade Center: the current price of every item, the weekly quota
// still available and how much is stored inside the building. Selling is automatic; the controller
// only closes the panel.
class TradePanelView {
public:
    explicit TradePanelView(core::EventBus& bus);

    void draw() const;

private:
    events::TradePanelChanged panel_;
    model::PlayerId localPlayerId_ = model::kNoPlayer;
};

} // namespace view::gameplay
