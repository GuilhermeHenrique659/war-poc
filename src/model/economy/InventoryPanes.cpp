#include "model/economy/InventoryPanes.hpp"

#include <utility>

namespace model::inventory_panes {

std::vector<events::InventorySlotState> slotStates(const components::Inventory& inventory)
{
    std::vector<events::InventorySlotState> slots;
    slots.reserve(inventory.slots.size());
    for (const auto& slot : inventory.slots) {
        if (!slot) {
            slots.emplace_back();
            continue;
        }
        slots.push_back(events::InventorySlotState{
            true, slot->item, slot->currentQuantity, slot->maxQuantity});
    }
    return slots;
}

events::InventoryPaneState slots(events::InventoryPaneRole role, ecs::Entity entity,
    std::string title, const components::Inventory& inventory)
{
    events::InventoryPaneState pane{};
    pane.role = role;
    pane.layout = events::InventoryPaneLayout::Slots;
    pane.entity = entity;
    pane.title = std::move(title);
    pane.showSummary = role == events::InventoryPaneRole::Player;
    pane.showTransferHint = role == events::InventoryPaneRole::Player
        || role == events::InventoryPaneRole::TransferTarget;
    pane.slots = slotStates(inventory);
    return pane;
}

events::InventoryPaneState units(std::vector<events::UnitInventoryState> selectedUnits)
{
    events::InventoryPaneState pane{};
    pane.role = events::InventoryPaneRole::Units;
    pane.layout = events::InventoryPaneLayout::Rows;
    pane.title = "Tropas selecionadas";
    pane.units = std::move(selectedUnits);
    return pane;
}

} // namespace model::inventory_panes
