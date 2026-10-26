#pragma once

#include "core/Events.hpp"
#include "model/build/BuildingType.hpp"
#include "model/economy/InventoryComponents.hpp"

#include <optional>
#include <vector>

namespace model::inventory_panes {

// Converts any entity inventory into the generic slot representation used by the panel.
[[nodiscard]] std::vector<events::InventorySlotState> slotStates(const components::Inventory& inventory);

// Player, building and unit panes share this structure; only role, layout and target type change.
[[nodiscard]] events::InventoryPaneState slots(events::InventoryPaneRole role, ecs::Entity entity,
    std::optional<BuildingType> buildingType, const components::Inventory& inventory);
[[nodiscard]] events::InventoryPaneState units(std::vector<events::UnitInventoryState> selectedUnits);

} // namespace model::inventory_panes
