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
};

} // namespace view::gameplay
