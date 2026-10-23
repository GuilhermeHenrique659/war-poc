#include "model/selection/SelectedUnitInventory.hpp"

#include "model/economy/InventoryComponents.hpp"
#include "model/economy/InventoryPanes.hpp"
#include "model/units/UnitComponents.hpp"
#include "model/units/UnitType.hpp"

#include <string>
#include <utility>

namespace model {

std::vector<events::UnitInventoryState> selectedUnitInventories(
    ecs::Registry& world, const std::vector<ecs::Entity>& units)
{
    std::vector<events::UnitInventoryState> result;
    result.reserve(units.size());
    for (const ecs::Entity entity : units) {
        if (!world.isAlive(entity)) {
            continue;
        }
        const auto* unit = world.tryGet<components::Unit>(entity);
        if (unit == nullptr) {
            continue;
        }
        const auto* health = world.tryGet<components::Health>(entity);
        const auto* inventory = world.tryGet<components::Inventory>(entity);

        events::UnitInventoryState entry{};
        entry.entity = entity;
        entry.label = std::string(unitSpec(unit->type).label);
        if (health != nullptr) {
            entry.health = health->current;
            entry.maxHealth = health->max;
        }
        if (inventory != nullptr) {
            entry.slots = inventory_panes::slotStates(*inventory);
        }
        result.push_back(std::move(entry));
    }
    return result;
}

} // namespace model
