#pragma once

#include "core/EventBus.hpp"
#include "core/Events.hpp"

#include <cstddef>

namespace view::gameplay {

class InventoryPanelView {
public:
    explicit InventoryPanelView(core::EventBus& bus);

    void draw() const;

private:
    void drawSlot(std::size_t index, bool playerInventory) const;
    void drawInventory(
        const std::vector<events::InventorySlotState>& slots, bool playerInventory) const;

    events::InventoryPanelChanged panel_;
    events::InventoryDragChanged drag_;
    model::PlayerId localPlayerId_ = model::kNoPlayer;
};

} // namespace view::gameplay
