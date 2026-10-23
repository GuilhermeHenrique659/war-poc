#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/build/BuildingComponents.hpp"
#include "model/build/BuildingSystem.hpp"
#include "model/economy/InventoryComponents.hpp"
#include "model/economy/InventorySystem.hpp"
#include "model/economy/Item.hpp"
#include "model/units/UnitComponents.hpp"
#include "model/units/UnitSupplySystem.hpp"
#include "model/units/UnitSystem.hpp"

#include <gtest/gtest.h>

namespace {

class UnitSupplySystemTest : public ::testing::Test {
protected:
    UnitSupplySystemTest()
        : buildings_(bus_, world_)
        , units_(bus_, world_, buildings_)
        , supplies_(world_, units_)
    {
        // Free placement keeps the tests focused on the supply rules.
        bus_.publish(events::DebugToggleRequested{});
    }

    [[nodiscard]] model::ecs::Entity place(model::BuildingType type, model::GridPos anchor)
    {
        buildings_.initializePlayer(kPlayer);
        // Placement keeps the type selected for repeated builds; clear it before choosing again.
        buildings_.cancel(kPlayer);
        buildings_.select(kPlayer, type);
        buildings_.place(kPlayer, anchor);
        model::ecs::Entity placed = 0;
        bool found = false;
        world_.each<model::components::Building>(
            [&](model::ecs::Entity entity, const model::components::Building& building) {
                if (building.type == type && (!found || entity > placed)) {
                    placed = entity;
                    found = true;
                }
            });
        EXPECT_TRUE(found);
        return placed;
    }

    void stock(model::ecs::Entity building, model::ItemId item, int quantity)
    {
        auto* inventory = world_.tryGet<model::components::Inventory>(building);
        ASSERT_NE(inventory, nullptr);
        EXPECT_EQ(model::InventorySystem::add(*inventory, item, quantity), quantity);
    }

    void run(int frames)
    {
        for (int frame = 0; frame < frames; ++frame) {
            supplies_.update();
            (void)units_.update(1.0F);
        }
    }

    [[nodiscard]] model::components::Inventory& inventoryOf(model::ecs::Entity entity)
    {
        auto* inventory = world_.tryGet<model::components::Inventory>(entity);
        EXPECT_NE(inventory, nullptr);
        return *inventory;
    }

    [[nodiscard]] const model::components::UnitSupplyState& stateOf(model::ecs::Entity entity)
    {
        const auto* state = world_.tryGet<model::components::UnitSupplyState>(entity);
        EXPECT_NE(state, nullptr);
        static const model::components::UnitSupplyState fallback{};
        return state == nullptr ? fallback : *state;
    }

    core::EventBus bus_;
    model::ecs::Registry world_;
    model::BuildingSystem buildings_;
    model::UnitSystem units_;
    model::UnitSupplySystem supplies_;
    static constexpr model::PlayerId kPlayer = 1;
};

} // namespace

TEST_F(UnitSupplySystemTest, NewUnitFetchesWeaponAndAmmoFromTheNearestStockedWarehouse)
{
    // Given two stocked warehouses and a command center without supplies.
    (void)place(model::BuildingType::CommandCenter, {40, 40});
    const auto nearWarehouse = place(model::BuildingType::Warehouse, {10, 10});
    const auto farWarehouse = place(model::BuildingType::Warehouse, {25, 25});
    stock(nearWarehouse, model::ItemId::Weapon, 5);
    stock(nearWarehouse, model::ItemId::Ammo, 500);
    stock(farWarehouse, model::ItemId::Weapon, 5);
    stock(farWarehouse, model::ItemId::Ammo, 500);

    // When a newly recruited Soldier searches for supplies.
    const auto soldier = units_.spawn(kPlayer, model::recruitStats(model::UnitType::Soldier), {0, 0});
    run(100);

    // Then it takes one weapon and a full ammo load only from the nearest warehouse.
    auto& inventory = inventoryOf(soldier);
    EXPECT_EQ(model::InventorySystem::amount(inventory, model::ItemId::Weapon), 1);
    EXPECT_EQ(model::InventorySystem::amount(inventory, model::ItemId::Ammo), model::kUnitAmmoCapacity);
    EXPECT_EQ(model::InventorySystem::amount(inventoryOf(nearWarehouse), model::ItemId::Weapon), 4);
    EXPECT_EQ(model::InventorySystem::amount(inventoryOf(nearWarehouse), model::ItemId::Ammo), 400);
    EXPECT_EQ(model::InventorySystem::amount(inventoryOf(farWarehouse), model::ItemId::Weapon), 5);
    EXPECT_EQ(model::InventorySystem::amount(inventoryOf(farWarehouse), model::ItemId::Ammo), 500);
    EXPECT_EQ(stateOf(soldier).activity, model::UnitActivity::Idle);
}

TEST_F(UnitSupplySystemTest, FallsBackToTheCommandCenterWhenThereIsNoWarehouse)
{
    // Given a stocked command center and no warehouse.
    const auto commandCenter = place(model::BuildingType::CommandCenter, {10, 10});
    stock(commandCenter, model::ItemId::Weapon, 1);
    stock(commandCenter, model::ItemId::Ammo, 100);

    // When a newly recruited Soldier searches for supplies.
    const auto soldier = units_.spawn(kPlayer, model::recruitStats(model::UnitType::Soldier), {0, 0});
    run(100);

    // Then it fills its inventory from the command center.
    EXPECT_EQ(model::InventorySystem::amount(inventoryOf(soldier), model::ItemId::Weapon), 1);
    EXPECT_EQ(model::InventorySystem::amount(inventoryOf(soldier), model::ItemId::Ammo), model::kUnitAmmoCapacity);
    EXPECT_EQ(model::InventorySystem::amount(inventoryOf(commandCenter), model::ItemId::Weapon), 0);
    EXPECT_EQ(model::InventorySystem::amount(inventoryOf(commandCenter), model::ItemId::Ammo), 0);
}

TEST_F(UnitSupplySystemTest, UsesTheCommandCenterWhenTheWarehouseIsEmpty)
{
    // Given an empty warehouse near the unit and a stocked command center farther away.
    const auto commandCenter = place(model::BuildingType::CommandCenter, {30, 30});
    const auto warehouse = place(model::BuildingType::Warehouse, {8, 8});
    stock(commandCenter, model::ItemId::Weapon, 1);
    stock(commandCenter, model::ItemId::Ammo, 100);

    // When a newly recruited Soldier searches for supplies.
    const auto soldier = units_.spawn(kPlayer, model::recruitStats(model::UnitType::Soldier), {0, 0});
    run(120);

    // Then it ignores the empty warehouse and refills from the command center.
    EXPECT_EQ(model::InventorySystem::amount(inventoryOf(soldier), model::ItemId::Weapon), 1);
    EXPECT_EQ(model::InventorySystem::amount(inventoryOf(soldier), model::ItemId::Ammo), model::kUnitAmmoCapacity);
    EXPECT_EQ(model::InventorySystem::amount(inventoryOf(warehouse), model::ItemId::Weapon), 0);
    EXPECT_EQ(model::InventorySystem::amount(inventoryOf(warehouse), model::ItemId::Ammo), 0);
}

TEST_F(UnitSupplySystemTest, StaysStillWhenNoWarehouseOrCommandCenterHasSupplies)
{
    // Given a command center without any weapon or ammo.
    (void)place(model::BuildingType::CommandCenter, {30, 30});
    const auto soldier = units_.spawn(kPlayer, model::recruitStats(model::UnitType::Soldier), {4, 4});

    // When the unit searches for supplies for several seconds.
    run(5);

    // Then it remains on its tile, without a path or supplies.
    const auto* position = world_.tryGet<model::components::Position>(soldier);
    const auto* mover = world_.tryGet<model::components::Mover>(soldier);
    ASSERT_NE(position, nullptr);
    ASSERT_NE(mover, nullptr);
    EXPECT_FLOAT_EQ(position->x, 4.5F);
    EXPECT_FLOAT_EQ(position->y, 4.5F);
    EXPECT_TRUE(mover->path.empty());
    EXPECT_EQ(model::InventorySystem::amount(inventoryOf(soldier), model::ItemId::Weapon), 0);
    EXPECT_EQ(model::InventorySystem::amount(inventoryOf(soldier), model::ItemId::Ammo), 0);
    EXPECT_EQ(stateOf(soldier).activity, model::UnitActivity::WaitingForSupplies);
}

TEST_F(UnitSupplySystemTest, RefillsAmmoAutomaticallyAfterItReachesZero)
{
    // Given a unit that already fetched its initial supplies from a nearby warehouse.
    (void)place(model::BuildingType::CommandCenter, {30, 30});
    const auto warehouse = place(model::BuildingType::Warehouse, {6, 6});
    stock(warehouse, model::ItemId::Weapon, 1);
    stock(warehouse, model::ItemId::Ammo, 300);
    const auto soldier = units_.spawn(kPlayer, model::recruitStats(model::UnitType::Soldier), {0, 0});
    run(60);
    ASSERT_EQ(model::InventorySystem::amount(inventoryOf(soldier), model::ItemId::Ammo), model::kUnitAmmoCapacity);
    EXPECT_EQ(model::InventorySystem::remove(inventoryOf(soldier), model::ItemId::Ammo, model::kUnitAmmoCapacity),
        model::kUnitAmmoCapacity);

    // When the unit notices it has no ammunition left.
    run(60);

    // Then it returns to the warehouse and reloads a full stack.
    EXPECT_EQ(model::InventorySystem::amount(inventoryOf(soldier), model::ItemId::Ammo), model::kUnitAmmoCapacity);
    EXPECT_EQ(model::InventorySystem::amount(inventoryOf(warehouse), model::ItemId::Ammo), 100);
    EXPECT_EQ(stateOf(soldier).activity, model::UnitActivity::Idle);
}
