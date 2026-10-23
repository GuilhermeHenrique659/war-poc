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
    void drawPane(const events::InventoryPaneState& pane, std::size_t paneIndex) const;
    void drawSlotsPane(const events::InventoryPaneState& pane, std::size_t paneIndex) const;
    void drawSlot(const events::InventoryPaneState& pane, std::size_t paneIndex, std::size_t index) const;
    void drawUnitsPane(const events::InventoryPaneState& pane, std::size_t paneIndex) const;
    void drawUnitRow(
        const events::InventoryPaneState& pane, std::size_t paneIndex, std::size_t index) const;

    events::InventoryPanelChanged panel_;
    events::InventoryDragChanged drag_;
    model::PlayerId localPlayerId_ = model::kNoPlayer;
};

} // namespace view::gameplay
