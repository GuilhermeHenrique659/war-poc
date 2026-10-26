#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/build/BuildingComponents.hpp"
#include "model/build/BuildingSystem.hpp"
#include "model/components/Owner.hpp"
#include "model/economy/InventoryComponents.hpp"
#include "model/economy/InventorySystem.hpp"
#include "model/economy/LogisticsComponents.hpp"
#include "model/economy/ProductionComponents.hpp"
#include "model/economy/LogisticsSystem.hpp"
#include "model/ecs/Registry.hpp"
#include "model/player/PlayerComponents.hpp"
#include "model/units/UnitComponents.hpp"
#include "model/units/UnitSystem.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <utility>

namespace {

class LogisticsSystemTest : public ::testing::Test {
protected:
    LogisticsSystemTest()
        : buildings_(bus_, world_)
        , units_(bus_, world_, buildings_)
        , logistics_(bus_, world_, units_)
    {
        const auto player = world_.create();
        world_.emplace<model::components::PlayerProfile>(player, kPlayer, "Tester");
        auto& inventory = world_.emplace<model::components::Inventory>(player, std::size_t{32});
        EXPECT_EQ(model::InventorySystem::add(inventory, model::ItemId::Iron, 500), 500);
        EXPECT_EQ(model::InventorySystem::add(inventory, model::ItemId::Electronics, 500), 500);
        bus_.subscribe<events::ResourceConsumed>(
            [this](const events::ResourceConsumed& event) { consumed_.push_back(event); });
    }

    model::ecs::Entity makeBuilding(
        model::BuildingType type, model::PlayerId owner, model::GridPos origin, model::ItemId output,
        std::vector<model::ProductionInput> inputs = {})
    {
        const auto entity = world_.create();
        const auto spec = model::buildingSpec(type);
        world_.emplace<model::components::Building>(entity, type);
        world_.emplace<model::components::Owner>(entity, owner);
        world_.emplace<model::components::Footprint>(entity, origin, spec.size);
        world_.emplace<model::components::Inventory>(entity, std::size_t{64});
        if (output != model::ItemId::Count) {
            world_.emplace<model::components::Production>(entity, std::move(inputs), output, 1.0F);
        }
        return entity;
    }

    void publishPlaced(model::ecs::Entity entity, model::BuildingType type, model::PlayerId owner)
    {
        const auto* footprint = world_.tryGet<model::components::Footprint>(entity);
        ASSERT_NE(footprint, nullptr);
        bus_.publish(events::BuildingPlaced{entity, owner, type, footprint->origin, footprint->size});
    }

    model::components::Inventory& playerInventory()
    {
        model::components::Inventory* inventory = nullptr;
        world_.each<model::components::PlayerProfile>([&](model::ecs::Entity entity,
            const model::components::PlayerProfile& profile) {
            if (profile.id == kPlayer) {
                inventory = world_.tryGet<model::components::Inventory>(entity);
            }
        });
        EXPECT_NE(inventory, nullptr);
        return *inventory;
    }

    core::EventBus bus_;
    model::ecs::Registry world_;
    model::BuildingSystem buildings_;
    model::UnitSystem units_;
    model::LogisticsSystem logistics_;
    std::vector<events::ResourceConsumed> consumed_;
    static constexpr model::PlayerId kPlayer = 1;
};

} // namespace

TEST_F(LogisticsSystemTest, ConnectsProducersToExistingWarehousesAndMatchingConsumers)
{
    // Given an existing warehouse and a factory that consumes iron.
    const auto warehouse = makeBuilding(model::BuildingType::Warehouse, kPlayer, {20, 20}, model::ItemId::Count);
    const auto factory = makeBuilding(model::BuildingType::Steelworks, kPlayer, {30, 30}, model::ItemId::Steel,
        {{model::ItemId::Iron, 1.0F}});
    publishPlaced(warehouse, model::BuildingType::Warehouse, kPlayer);
    publishPlaced(factory, model::BuildingType::Steelworks, kPlayer);

    // When an iron producer is built.
    const auto mine = makeBuilding(model::BuildingType::IronMine, kPlayer, {0, 0}, model::ItemId::Iron);
    publishPlaced(mine, model::BuildingType::IronMine, kPlayer);

    // Then routes are owned by the source and target both the warehouse and matching consumer.
    const auto* routes = world_.tryGet<model::components::LogisticsRoutes>(mine);
    ASSERT_NE(routes, nullptr);
    ASSERT_EQ(routes->routes.size(), 2U);
    EXPECT_EQ(routes->routes[0].source, mine);
    EXPECT_EQ(routes->routes[0].item, model::ItemId::Iron);
    EXPECT_TRUE((routes->routes[0].destination == warehouse || routes->routes[0].destination == factory));
    EXPECT_TRUE((routes->routes[1].destination == warehouse || routes->routes[1].destination == factory));
    EXPECT_NE(routes->routes[0].destination, routes->routes[1].destination);
}

TEST_F(LogisticsSystemTest, RecruitsIdleDronesAtWarehousesAndAssignsThemToOwnedRoutes)
{
    // Given a warehouse and a producer with an automatic route to it.
    const auto warehouse = makeBuilding(model::BuildingType::Warehouse, kPlayer, {10, 10}, model::ItemId::Count);
    publishPlaced(warehouse, model::BuildingType::Warehouse, kPlayer);
    const auto mine = makeBuilding(model::BuildingType::IronMine, kPlayer, {0, 0}, model::ItemId::Iron);
    publishPlaced(mine, model::BuildingType::IronMine, kPlayer);
    const auto* routes = world_.tryGet<model::components::LogisticsRoutes>(mine);
    ASSERT_NE(routes, nullptr);
    ASSERT_EQ(routes->routes.size(), 1U);
    const auto routeId = routes->routes.front().id;

    // When the player recruits a drone and assigns it to the route.
    const auto drone = logistics_.recruitDrone(kPlayer, warehouse);
    ASSERT_TRUE(drone.has_value());
    EXPECT_EQ(model::InventorySystem::amount(playerInventory(), model::ItemId::Iron), 450);
    EXPECT_EQ(model::InventorySystem::amount(playerInventory(), model::ItemId::Electronics), 450);
    EXPECT_TRUE(logistics_.assignIdleDrone(kPlayer, mine, routeId));

    // Then the drone has a ten-slot cargo inventory and the route reports its assignment.
    const auto* cargo = world_.tryGet<model::components::Inventory>(*drone);
    ASSERT_NE(cargo, nullptr);
    EXPECT_EQ(cargo->slots.size(), 10U);
    EXPECT_EQ(model::InventorySystem::availableCapacity(*cargo, model::ItemId::Iron), 1000);
    routes = world_.tryGet<model::components::LogisticsRoutes>(mine);
    ASSERT_NE(routes, nullptr);
    ASSERT_EQ(routes->routes.size(), 1U);
    EXPECT_EQ(routes->routes.front().assignedDrones, 1);
}

TEST_F(LogisticsSystemTest, CreatesAutomaticRoutesFromProducersToTradeCenters)
{
    // Given a Trade Center owned by the player.
    const auto center = makeBuilding(model::BuildingType::TradeCenter, kPlayer, {20, 20}, model::ItemId::Count);
    publishPlaced(center, model::BuildingType::TradeCenter, kPlayer);

    // When the player places an iron mine.
    const auto mine = makeBuilding(model::BuildingType::IronMine, kPlayer, {0, 0}, model::ItemId::Iron);
    publishPlaced(mine, model::BuildingType::IronMine, kPlayer);

    // Then the mine delivers its iron to the Trade Center.
    const auto* routes = world_.tryGet<model::components::LogisticsRoutes>(mine);
    ASSERT_NE(routes, nullptr);
    ASSERT_EQ(routes->routes.size(), 1U);
    EXPECT_EQ(routes->routes[0].destination, center);
    EXPECT_EQ(routes->routes[0].item, model::ItemId::Iron);
}

TEST_F(LogisticsSystemTest, ConnectsExistingProducersWhenATradeCenterIsPlaced)
{
    // Given an iron mine already placed.
    const auto mine = makeBuilding(model::BuildingType::IronMine, kPlayer, {0, 0}, model::ItemId::Iron);
    publishPlaced(mine, model::BuildingType::IronMine, kPlayer);

    // When the player places a Trade Center.
    const auto center = makeBuilding(model::BuildingType::TradeCenter, kPlayer, {20, 20}, model::ItemId::Count);
    publishPlaced(center, model::BuildingType::TradeCenter, kPlayer);

    // Then the existing mine gets a route to the new Trade Center.
    const auto* routes = world_.tryGet<model::components::LogisticsRoutes>(mine);
    ASSERT_NE(routes, nullptr);
    ASSERT_EQ(routes->routes.size(), 1U);
    EXPECT_EQ(routes->routes[0].destination, center);
    EXPECT_EQ(routes->routes[0].item, model::ItemId::Iron);
}

TEST_F(LogisticsSystemTest, RecruitReportsTheConsumedMaterials)
{
    // Given an owned warehouse and enough materials.
    const auto warehouse = makeBuilding(model::BuildingType::Warehouse, kPlayer, {10, 10}, model::ItemId::Count);
    consumed_.clear();

    // When the player recruits a transport drone.
    ASSERT_TRUE(logistics_.recruitDrone(kPlayer, warehouse).has_value());

    // Then the paid iron and electronics are reported to the economy.
    ASSERT_EQ(consumed_.size(), 2U);
    EXPECT_EQ(consumed_[0].player, kPlayer);
    EXPECT_EQ(consumed_[0].item, model::ItemId::Iron);
    EXPECT_EQ(consumed_[0].quantity, model::components::kTransportDroneIronCost);
    EXPECT_EQ(consumed_[1].item, model::ItemId::Electronics);
    EXPECT_EQ(consumed_[1].quantity, model::components::kTransportDroneElectronicsCost);
}

TEST_F(LogisticsSystemTest, DoesNotRecruitDroneWhenPlayerCannotAffordIt)
{
    // Given a warehouse and enough iron but only 49 electronics.
    const auto warehouse = makeBuilding(model::BuildingType::Warehouse, kPlayer, {10, 10}, model::ItemId::Count);
    auto& inventory = playerInventory();
    EXPECT_EQ(model::InventorySystem::remove(inventory, model::ItemId::Electronics, 451), 451);

    // When the player tries to recruit a drone.
    const auto drone = logistics_.recruitDrone(kPlayer, warehouse);

    // Then no drone is created and neither resource is spent.
    EXPECT_FALSE(drone.has_value());
    EXPECT_EQ(model::InventorySystem::amount(inventory, model::ItemId::Iron), 500);
    EXPECT_EQ(model::InventorySystem::amount(inventory, model::ItemId::Electronics), 49);
    int droneCount = 0;
    world_.each<model::components::TransportDrone>(
        [&droneCount](model::ecs::Entity, const model::components::TransportDrone&) { ++droneCount; });
    EXPECT_EQ(droneCount, 0);
}

TEST_F(LogisticsSystemTest, DebugModeMakesDroneRecruitmentFree)
{
    // Given an owned warehouse and a player with no iron or electronics.
    const auto warehouse = makeBuilding(model::BuildingType::Warehouse, kPlayer, {10, 10}, model::ItemId::Count);
    auto& inventory = playerInventory();
    EXPECT_EQ(model::InventorySystem::remove(inventory, model::ItemId::Iron, 500), 500);
    EXPECT_EQ(model::InventorySystem::remove(inventory, model::ItemId::Electronics, 500), 500);

    // When debug mode is enabled and the player recruits a drone.
    bus_.publish(events::DebugToggleRequested{});
    const auto drone = logistics_.recruitDrone(kPlayer, warehouse);

    // Then the drone is created without spending resources.
    ASSERT_TRUE(drone.has_value());
    EXPECT_EQ(model::InventorySystem::amount(inventory, model::ItemId::Iron), 0);
    EXPECT_EQ(model::InventorySystem::amount(inventory, model::ItemId::Electronics), 0);
}

TEST_F(LogisticsSystemTest, UnassignsDroneAndReturnsItsCargoToTheRouteSource)
{
    // Given an assigned drone carrying iron from its route.
    const auto warehouse = makeBuilding(model::BuildingType::Warehouse, kPlayer, {10, 10}, model::ItemId::Count);
    publishPlaced(warehouse, model::BuildingType::Warehouse, kPlayer);
    const auto mine = makeBuilding(model::BuildingType::IronMine, kPlayer, {0, 0}, model::ItemId::Iron);
    publishPlaced(mine, model::BuildingType::IronMine, kPlayer);
    const auto* routes = world_.tryGet<model::components::LogisticsRoutes>(mine);
    ASSERT_NE(routes, nullptr);
    ASSERT_EQ(routes->routes.size(), 1U);
    const auto routeId = routes->routes.front().id;
    const auto drone = logistics_.recruitDrone(kPlayer, warehouse);
    ASSERT_TRUE(drone.has_value());
    ASSERT_TRUE(logistics_.assignIdleDrone(kPlayer, mine, routeId));
    auto* cargo = world_.tryGet<model::components::Inventory>(*drone);
    auto* sourceInventory = world_.tryGet<model::components::Inventory>(mine);
    ASSERT_NE(cargo, nullptr);
    ASSERT_NE(sourceInventory, nullptr);
    EXPECT_EQ(model::InventorySystem::add(*cargo, model::ItemId::Iron, 75), 75);

    // When the player unassigns the drone and it returns to its source.
    ASSERT_TRUE(logistics_.unassignDrone(kPlayer, mine, routeId));
    for (int frame = 0; frame < 100; ++frame) {
        (void)units_.update(1.0F);
        logistics_.update();
        const auto* transport = world_.tryGet<model::components::TransportDrone>(*drone);
        if (transport != nullptr && transport->state == model::components::DroneTransportState::Idle) {
            break;
        }
    }

    // Then the route is unassigned and the carried items are returned instead of lost.
    routes = world_.tryGet<model::components::LogisticsRoutes>(mine);
    ASSERT_NE(routes, nullptr);
    EXPECT_EQ(routes->routes.front().assignedDrones, 0);
    const auto* transport = world_.tryGet<model::components::TransportDrone>(*drone);
    ASSERT_NE(transport, nullptr);
    EXPECT_FALSE(transport->routeId.has_value());
    EXPECT_EQ(transport->state, model::components::DroneTransportState::Idle);
    EXPECT_EQ(model::InventorySystem::amount(*sourceInventory, model::ItemId::Iron), 75);
    EXPECT_EQ(model::InventorySystem::amount(*cargo, model::ItemId::Iron), 0);
}

TEST_F(LogisticsSystemTest, AddsRoutesToNewWarehousesAndNewConsumers)
{
    // Given an existing iron mine with no destinations yet.
    const auto mine = makeBuilding(model::BuildingType::IronMine, kPlayer, {0, 0}, model::ItemId::Iron);
    publishPlaced(mine, model::BuildingType::IronMine, kPlayer);
    EXPECT_EQ(world_.tryGet<model::components::LogisticsRoutes>(mine), nullptr);

    // When a warehouse and a matching factory are built later.
    const auto warehouse = makeBuilding(model::BuildingType::Warehouse, kPlayer, {10, 10}, model::ItemId::Count);
    publishPlaced(warehouse, model::BuildingType::Warehouse, kPlayer);
    const auto factory = makeBuilding(model::BuildingType::Steelworks, kPlayer, {20, 20}, model::ItemId::Steel,
        {{model::ItemId::Iron, 1.0F}});
    publishPlaced(factory, model::BuildingType::Steelworks, kPlayer);

    // Then both later destinations receive routes stored on the original producer.
    const auto* routes = world_.tryGet<model::components::LogisticsRoutes>(mine);
    ASSERT_NE(routes, nullptr);
    ASSERT_EQ(routes->routes.size(), 2U);
    EXPECT_TRUE(std::any_of(routes->routes.begin(), routes->routes.end(),
        [warehouse](const auto& route) { return route.destination == warehouse; }));
    EXPECT_TRUE(std::any_of(routes->routes.begin(), routes->routes.end(),
        [factory](const auto& route) { return route.destination == factory; }));
}

TEST_F(LogisticsSystemTest, TransportsCargoAlongTheRouteAndRetainsCargoWhenDestinationIsFull)
{
    // Given a route with iron at its source and only ten units of free destination capacity.
    const auto warehouse = makeBuilding(model::BuildingType::Warehouse, kPlayer, {12, 12}, model::ItemId::Count);
    publishPlaced(warehouse, model::BuildingType::Warehouse, kPlayer);
    const auto mine = makeBuilding(model::BuildingType::IronMine, kPlayer, {0, 0}, model::ItemId::Iron);
    publishPlaced(mine, model::BuildingType::IronMine, kPlayer);
    auto* sourceInventory = world_.tryGet<model::components::Inventory>(mine);
    auto* warehouseInventory = world_.tryGet<model::components::Inventory>(warehouse);
    ASSERT_NE(sourceInventory, nullptr);
    ASSERT_NE(warehouseInventory, nullptr);
    warehouseInventory->slots.resize(1);
    EXPECT_EQ(model::InventorySystem::add(*sourceInventory, model::ItemId::Iron, 150), 150);
    EXPECT_EQ(model::InventorySystem::add(*warehouseInventory, model::ItemId::Iron, 90), 90);
    const auto* routes = world_.tryGet<model::components::LogisticsRoutes>(mine);
    ASSERT_NE(routes, nullptr);
    ASSERT_EQ(routes->routes.size(), 1U);

    // When a drone is assigned, it travels to the mine, loads cargo, and follows the route.
    const auto drone = logistics_.recruitDrone(kPlayer, warehouse);
    ASSERT_TRUE(drone.has_value());
    ASSERT_TRUE(logistics_.assignIdleDrone(kPlayer, mine, routes->routes.front().id));
    for (int frame = 0; frame < 100; ++frame) {
        (void)units_.update(1.0F);
        logistics_.update();
        if (model::InventorySystem::amount(*warehouseInventory, model::ItemId::Iron) == 100) {
            break;
        }
    }

    // Then the destination accepts what fits and the drone retains the blocked remainder.
    EXPECT_EQ(model::InventorySystem::amount(*sourceInventory, model::ItemId::Iron), 0);
    EXPECT_EQ(model::InventorySystem::amount(*warehouseInventory, model::ItemId::Iron), 100);
    const auto* cargo = world_.tryGet<model::components::Inventory>(*drone);
    ASSERT_NE(cargo, nullptr);
    EXPECT_EQ(model::InventorySystem::amount(*cargo, model::ItemId::Iron), 140);
    const auto* transport = world_.tryGet<model::components::TransportDrone>(*drone);
    ASSERT_NE(transport, nullptr);
    EXPECT_EQ(transport->state, model::components::DroneTransportState::WaitingAtDestination);
}

TEST_F(LogisticsSystemTest, PublishesDroneLoadStateWhenCargoIsLoadedAndUnloaded)
{
    // Given a route whose source has cargo and whose destination has room.
    const auto warehouse = makeBuilding(model::BuildingType::Warehouse, kPlayer, {12, 12}, model::ItemId::Count);
    publishPlaced(warehouse, model::BuildingType::Warehouse, kPlayer);
    const auto mine = makeBuilding(model::BuildingType::IronMine, kPlayer, {0, 0}, model::ItemId::Iron);
    publishPlaced(mine, model::BuildingType::IronMine, kPlayer);
    auto* sourceInventory = world_.tryGet<model::components::Inventory>(mine);
    auto* warehouseInventory = world_.tryGet<model::components::Inventory>(warehouse);
    ASSERT_NE(sourceInventory, nullptr);
    ASSERT_NE(warehouseInventory, nullptr);
    EXPECT_EQ(model::InventorySystem::add(*sourceInventory, model::ItemId::Iron, 40), 40);

    const auto* routes = world_.tryGet<model::components::LogisticsRoutes>(mine);
    ASSERT_NE(routes, nullptr);
    ASSERT_EQ(routes->routes.size(), 1U);
    std::vector<std::pair<model::ecs::Entity, events::DroneLoadState>> observedStates;
    bus_.subscribe<events::DroneLoadStateChanged>([&](const events::DroneLoadStateChanged& event) {
        observedStates.emplace_back(event.entity, event.state);
    });
    const auto drone = logistics_.recruitDrone(kPlayer, warehouse);
    ASSERT_TRUE(drone.has_value());

    // When the drone is assigned and runs until it has delivered its cargo.
    ASSERT_TRUE(logistics_.assignIdleDrone(kPlayer, mine, routes->routes.front().id));
    for (int frame = 0; frame < 100; ++frame) {
        (void)units_.update(1.0F);
        logistics_.update();
        if (model::InventorySystem::amount(*warehouseInventory, model::ItemId::Iron) == 40) {
            break;
        }
    }

    // Then the bus reports each visible inventory-state transition in order.
    std::vector<events::DroneLoadState> droneStates;
    for (const auto& [entity, state] : observedStates) {
        if (entity == *drone) {
            droneStates.push_back(state);
        }
    }
    ASSERT_EQ(droneStates.size(), 3U);
    EXPECT_EQ(droneStates[0], events::DroneLoadState::Unloaded);
    EXPECT_EQ(droneStates[1], events::DroneLoadState::Loaded);
    EXPECT_EQ(droneStates[2], events::DroneLoadState::Unloaded);
}
