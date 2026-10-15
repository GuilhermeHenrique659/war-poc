#pragma once

#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/Player.hpp"

namespace view {

class EconomyPanelView {
public:
    explicit EconomyPanelView(core::EventBus& bus);

    void draw() const;

private:
    model::PlayerId localPlayerId_ = model::kNoPlayer;
    events::EconomyChanged economy_{};
    bool hasEconomy_ = false;
};

} // namespace view
