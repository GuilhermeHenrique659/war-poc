#include "model/economy/InventoryPanelSystem.hpp"

#include "model/build/BuildingComponents.hpp"
#include "model/components/Owner.hpp"
#include "model/economy/InventoryComponents.hpp"
#include "model/economy/InventoryPanes.hpp"
#include "model/economy/InventorySystem.hpp"
#include "model/player/PlayerInventory.hpp"
#include "model/selection/SelectedUnitInventory.hpp"

namespace model {

InventoryPanelSystem::InventoryPanelSystem(core::EventBus& bus, ecs::Registry& world,
    SelectionSystem& selection, PlayerAvatarSystem& players)
    : bus_(bus)
    , world_(world)
    , selection_(selection)
    , players_(players)
{
    bus_.subscribe<events::SelectionChanged>(
        [this](const events::SelectionChanged& event) { onSelectionChanged(event); });
    bus_.subscribe<events::TogglePlayerInventoryRequested>(
        [this](const events::TogglePlayerInventoryRequested& event) {
            if (!players_.entity(event.player)) {
                return;
            }
            playerInventoryOpen_[event.player] = !playerInventoryOpen_[event.player];
            publish(event.player);
        });
    bus_.subscribe<events::CloseInventoryRequested>(
        [this](const events::CloseInventoryRequested& event) {
            if (!players_.entity(event.player)) {
                return;
            }
            playerInventoryOpen_[event.player] = false;
            buildingPanels_.erase(event.player);
            unitPanels_.erase(event.player);
            publish(event.player);
        });
}

void InventoryPanelSystem::reset()
{
    buildingPanels_.clear();
    unitPanels_.clear();
    playerInventoryOpen_.clear();
}

void InventoryPanelSystem::update() const
{
    for (const auto& [player, building] : buildingPanels_) {
        (void)building;
        publish(player);
    }
    for (const PlayerId player : unitPanels_) {
        if (!buildingPanels_.contains(player)) {
            publish(player);
        }
    }
}

bool InventoryPanelSystem::transfer(
    PlayerId player, ecs::Entity building, std::size_t slotIndex, bool fromPlayer)
{
    const auto panel = buildingPanels_.find(player);
    const auto playerEntity = players_.entity(player);
    if (panel == buildingPanels_.end() || panel->second != building || !playerEntity) {
        return false;
    }
    const auto* owner = world_.tryGet<components::Owner>(building);
    auto* buildingInventory = world_.tryGet<components::Inventory>(building);
    auto* playerInventory = model::playerInventory(world_, player);
    if (owner == nullptr || owner->player != player || buildingInventory == nullptr
        || playerInventory == nullptr) {
        return false;
    }
    const auto& sourceSlots = fromPlayer ? playerInventory->slots : buildingInventory->slots;
    if (slotIndex >= sourceSlots.size() || !sourceSlots[slotIndex]) {
        return false;
    }
    const auto& slot = sourceSlots[slotIndex];
    if (fromPlayer) {
        (void)InventorySystem::transfer(
            *playerInventory, *buildingInventory, slot->item, slot->currentQuantity);
    } else {
        (void)InventorySystem::transfer(
            *buildingInventory, *playerInventory, slot->item, slot->currentQuantity);
    }
    publish(player);
    return true;
}

void InventoryPanelSystem::onSelectionChanged(const events::SelectionChanged& event)
{
    buildingPanels_.erase(event.player);
    if (event.building) {
        const auto* owner = world_.tryGet<components::Owner>(*event.building);
        if (owner != nullptr && owner->player == event.player
            && world_.has<components::Building>(*event.building)
            && world_.has<components::Inventory>(*event.building)) {
            buildingPanels_[event.player] = *event.building;
        }
    }
    if (event.units.empty()) {
        unitPanels_.erase(event.player);
    } else {
        unitPanels_.insert(event.player);
    }
    publish(event.player);
}

void InventoryPanelSystem::publish(PlayerId player) const
{
    const auto playerEntity = players_.entity(player);
    const auto* playerInventory = model::playerInventory(world_, player);
    const auto buildingPanel = buildingPanels_.find(player);
    const auto* building = buildingPanel == buildingPanels_.end()
        ? nullptr
        : world_.tryGet<components::Building>(buildingPanel->second);
    const auto* buildingInventory = buildingPanel == buildingPanels_.end()
        ? nullptr
        : world_.tryGet<components::Inventory>(buildingPanel->second);
    const bool hasBuilding = building != nullptr && buildingInventory != nullptr;
    const bool hasUnits = unitPanels_.contains(player);
    const auto openPanel = playerInventoryOpen_.find(player);
    const bool playerInventoryOpen = openPanel != playerInventoryOpen_.end() && openPanel->second;

    if (playerInventory == nullptr || (!hasBuilding && !hasUnits && !playerInventoryOpen)) {
        events::InventoryPanelChanged closed{};
        closed.player = player;
        bus_.publish(closed);
        return;
    }

    events::InventoryPanelChanged event{};
    event.player = player;
    event.open = true;
    event.playerInventoryOpen = playerInventoryOpen;
    event.panes.push_back(inventory_panes::slots(
        events::InventoryPaneRole::Player, *playerEntity, std::nullopt, *playerInventory));
    if (hasBuilding) {
        event.panes.push_back(inventory_panes::slots(events::InventoryPaneRole::TransferTarget,
            buildingPanel->second, building->type, *buildingInventory));
    }
    if (hasUnits) {
        event.panes.push_back(
            inventory_panes::units(selectedUnitInventories(world_, selection_.selectedUnits(player))));
    }
    bus_.publish(event);
}

} // namespace model
