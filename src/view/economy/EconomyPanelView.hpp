#pragma once

#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/player/Player.hpp"

namespace view::gameplay {

class EconomyPanelView {
public:
    explicit EconomyPanelView(core::EventBus& bus);

    void draw() const;

private:
    model::PlayerId localPlayerId_ = model::kNoPlayer;
    events::EconomyChanged economy_{};
    bool hasEconomy_ = false;
};

} // namespace view::gameplay
