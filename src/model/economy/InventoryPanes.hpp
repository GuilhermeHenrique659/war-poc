#pragma once

#include "core/Events.hpp"
#include "model/economy/InventoryComponents.hpp"

#include <string>
#include <vector>

namespace model::inventory_panes {

// Converts any entity inventory into the generic slot representation used by the panel.
[[nodiscard]] std::vector<events::InventorySlotState> slotStates(const components::Inventory& inventory);

// Player, building and unit panes share this structure; only role, layout and title change.
[[nodiscard]] events::InventoryPaneState slots(events::InventoryPaneRole role, ecs::Entity entity,
    std::string title, const components::Inventory& inventory);
[[nodiscard]] events::InventoryPaneState units(std::vector<events::UnitInventoryState> selectedUnits);

} // namespace model::inventory_panes
