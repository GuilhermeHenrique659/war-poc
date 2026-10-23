#pragma once

#include "model/economy/InventoryComponents.hpp"
#include "model/ecs/Registry.hpp"
#include "model/units/UnitComponents.hpp"
#include "model/units/UnitSystem.hpp"

#include <optional>

namespace model {

// State machine for automatic resupply: a unit seeks a stocked Warehouse (preferred) or
// Command Center, takes one weapon and a full ammo load, and waits in place when nothing
// available can supply it.
class UnitSupplySystem {
public:
    UnitSupplySystem(ecs::Registry& world, UnitSystem& units);

    void update();

private:
    void updateUnit(ecs::Entity entity);
    [[nodiscard]] std::optional<ecs::Entity> findSupplyTarget(
        ecs::Entity unit, bool needsWeapon, bool needsAmmo) const;
    [[nodiscard]] bool isNearSupply(ecs::Entity unit, ecs::Entity supply) const;
    void refill(components::Inventory& inventory, ecs::Entity supply) const;
    void stopIfMoving(ecs::Entity entity) const;

    ecs::Registry& world_;
    UnitSystem& units_;
};

} // namespace model
