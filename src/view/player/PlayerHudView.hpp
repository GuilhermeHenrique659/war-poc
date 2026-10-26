#pragma once

#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/player/Player.hpp"

namespace view::gameplay {

class PlayerHudView {
public:
    explicit PlayerHudView(core::EventBus& bus);

    void draw() const;

private:
    model::PlayerId localPlayerId_ = model::kNoPlayer;
    events::PlayerSpawned player_{};
    bool hasPlayer_ = false;
    double credit_ = 0.0;
    double weeklyInterest_ = 0.0;
    bool hasCredit_ = false;
};

} // namespace view::gameplay
