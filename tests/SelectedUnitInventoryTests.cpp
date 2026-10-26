#include "core/Events.hpp"
#include "model/economy/InventoryComponents.hpp"
#include "model/economy/InventorySystem.hpp"
#include "model/economy/Item.hpp"
#include "model/ecs/Registry.hpp"
#include "model/selection/SelectedUnitInventory.hpp"
#include "model/units/UnitComponents.hpp"
#include "model/units/UnitType.hpp"

#include <gtest/gtest.h>

namespace {

model::ecs::Entity spawnUnit(model::ecs::Registry& world, model::UnitType type, int health, int maxHealth,
    int weapons, int ammo)
{
    const model::ecs::Entity entity = world.create();
    world.emplace<model::components::Unit>(entity, type, 1, 1);
    world.emplace<model::components::Health>(entity, health, maxHealth);
    auto& inventory
        = world.emplace<model::components::Inventory>(entity, model::kUnitInventorySlots);
    (void)model::InventorySystem::add(inventory, model::ItemId::Weapon, weapons);
    (void)model::InventorySystem::add(inventory, model::ItemId::Ammo, ammo);
    return entity;
}

int amountOf(const events::UnitInventoryState& unit, model::ItemId item)
{
    int amount = 0;
    for (const auto& slot : unit.slots) {
        if (slot.occupied && slot.item == item) {
            amount += slot.currentQuantity;
        }
    }
    return amount;
}

} // namespace

TEST(SelectedUnitInventoryTests, ReportsWeaponAndAmmoForEachSelectedUnit)
{
    // Given a wounded soldier carrying a weapon and partial ammo.
    model::ecs::Registry world;
    const model::ecs::Entity soldier
        = spawnUnit(world, model::UnitType::Soldier, 80, 100, model::kUnitWeaponCount, 37);

    // When the inventories of the selected units are collected.
    const auto inventories = model::selectedUnitInventories(world, {soldier});

    // Then the unit's type, health and inventory slots are reported.
    ASSERT_EQ(inventories.size(), 1U);
    EXPECT_EQ(inventories[0].entity, soldier);
    EXPECT_EQ(inventories[0].type, model::UnitType::Soldier);
    EXPECT_EQ(inventories[0].health, 80);
    EXPECT_EQ(inventories[0].maxHealth, 100);
    ASSERT_EQ(inventories[0].slots.size(), model::kUnitInventorySlots);
    EXPECT_EQ(amountOf(inventories[0], model::ItemId::Weapon), model::kUnitWeaponCount);
    EXPECT_EQ(amountOf(inventories[0], model::ItemId::Ammo), 37);
}

TEST(SelectedUnitInventoryTests, ReportsMissingWeaponAndAmmo)
{
    // Given a unit that has not resupplied yet.
    model::ecs::Registry world;
    const model::ecs::Entity goat = spawnUnit(world, model::UnitType::Goat, 140, 140, 0, 0);

    // When its inventory is collected.
    const auto inventories = model::selectedUnitInventories(world, {goat});

    // Then the empty slots are explicit instead of being hidden.
    ASSERT_EQ(inventories.size(), 1U);
    EXPECT_EQ(inventories[0].type, model::UnitType::Goat);
    EXPECT_EQ(amountOf(inventories[0], model::ItemId::Weapon), 0);
    EXPECT_EQ(amountOf(inventories[0], model::ItemId::Ammo), 0);
}

TEST(SelectedUnitInventoryTests, SkipsEntitiesThatAreNotLiveUnits)
{
    // Given one live unit, one destroyed unit and one entity without the Unit component.
    model::ecs::Registry world;
    const model::ecs::Entity live = spawnUnit(world, model::UnitType::Soldier, 100, 100, 1, 100);
    const model::ecs::Entity destroyed = spawnUnit(world, model::UnitType::Bat, 60, 60, 1, 100);
    world.destroy(destroyed);
    const model::ecs::Entity notAUnit = world.create();
    world.emplace<model::components::Health>(notAUnit, 20, 20);

    // When the selection still references the removed entities.
    const auto inventories = model::selectedUnitInventories(world, {live, destroyed, notAUnit});

    // Then only the live unit is reported.
    ASSERT_EQ(inventories.size(), 1U);
    EXPECT_EQ(inventories[0].entity, live);
}
