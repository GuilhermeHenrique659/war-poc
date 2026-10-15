#pragma once

#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/Player.hpp"

#include <cstddef>
#include <vector>

namespace view {

// Side panel shown while the local player's Command Center is selected.
class RecruitPanelView {
public:
    explicit RecruitPanelView(core::EventBus& bus);

    void draw() const;

private:
    void drawHeader() const;
    void drawSlot(std::size_t index) const;

    bool open_ = false;
    std::vector<events::RecruitEntry> entries_;
    model::PlayerId localPlayerId_ = model::kNoPlayer;
};

} // namespace view
