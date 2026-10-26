#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/build/BuildingComponents.hpp"
#include "model/build/BuildingSystem.hpp"
#include "model/build/ConstructionComponents.hpp"
#include "model/build/ConstructionDroneSystem.hpp"
#include "model/build/ConstructionSystem.hpp"
#include "model/components/Owner.hpp"
#include "model/economy/InventoryComponents.hpp"
#include "model/economy/InventorySystem.hpp"
#include "model/economy/Item.hpp"
#include "model/player/PlayerComponents.hpp"
#include "model/units/UnitComponents.hpp"
#include "model/units/UnitSystem.hpp"
#include "ConstructionTestHelper.hpp"

#include <gtest/gtest.h>

#include <functional>
#include <vector>

namespace {

class ConstructionDroneTest : public ::testing::Test {
protected:
    ConstructionDroneTest()
        : buildings_(bus_, world_)
        , units_(bus_, world_, buildings_)
        , construction_(bus_, world_)
        , drones_(bus_, world_, units_)
    {
        bus_.subscribe<events::BuildingPlaced>([this](const events::BuildingPlaced& event) {
            placements_.push_back(event);
        });
        bus_.subscribe<events::ConstructionStarted>([this](const events::ConstructionStarted& event) {
            sites_.push_back(event);
        });
        bus_.subscribe<events::DroneSpawned>([this](const events::DroneSpawned& event) {
            spawns_.push_back(event);
        });

        const auto playerEntity = world_.create();
        world_.emplace<model::components::PlayerProfile>(playerEntity, kPlayer, "Tester");
        world_.emplace<model::components::Owner>(playerEntity, kPlayer);
        auto& inventory = world_.emplace<model::components::Inventory>(playerEntity, std::size_t{32});
        (void)model::InventorySystem::add(inventory, model::ItemId::Iron, 500);
        (void)model::InventorySystem::add(inventory, model::ItemId::Electronics, 100);
    }

    [[nodiscard]] model::ecs::Entity place(model::BuildingType type, model::GridPos anchor)
    {
        return test_support::placeAndComplete(buildings_, bus_, world_, kPlayer, type, anchor);
    }

    [[nodiscard]] model::ecs::Entity startSite(model::BuildingType type, model::GridPos anchor)
    {
        const std::size_t before = sites_.size();
        buildings_.cancel(kPlayer);
        buildings_.select(kPlayer, type);
        buildings_.place(kPlayer, anchor);
        EXPECT_EQ(sites_.size(), before + 1);
        return sites_.back().entity;
    }

    void stock(model::ecs::Entity entity, model::ItemId item, int quantity)
    {
        auto* inventory = world_.tryGet<model::components::Inventory>(entity);
        ASSERT_NE(inventory, nullptr);
        EXPECT_EQ(model::InventorySystem::add(*inventory, item, quantity), quantity);
    }

    [[nodiscard]] int amountIn(model::ecs::Entity entity, model::ItemId item)
    {
        const auto* inventory = world_.tryGet<model::components::Inventory>(entity);
        return inventory == nullptr ? -1 : model::InventorySystem::amount(*inventory, item);
    }

    [[nodiscard]] int droneCount()
    {
        int count = 0;
        world_.each<model::components::ConstructionDrone>(
            [&count](model::ecs::Entity, const model::components::ConstructionDrone&) { ++count; });
        return count;
    }

    void runUntil(const std::function<bool()>& done, float maxSeconds, float step = 0.05F)
    {
        const int steps = static_cast<int>(maxSeconds / step);
        for (int i = 0; i < steps; ++i) {
            if (done()) {
                return;
            }
            units_.update(step);
            drones_.update();
            construction_.update(step);
        }
    }

    core::EventBus bus_;
    model::ecs::Registry world_;
    model::BuildingSystem buildings_;
    model::UnitSystem units_;
    model::ConstructionSystem construction_;
    model::ConstructionDroneSystem drones_;
    std::vector<events::BuildingPlaced> placements_;
    std::vector<events::ConstructionStarted> sites_;
    std::vector<events::DroneSpawned> spawns_;
    static constexpr model::PlayerId kPlayer = 1;
};

} // namespace

TEST_F(ConstructionDroneTest, GrantsOneFreeConstructionDroneWithTheCommandCenter)
{
    // Given a player placing their Command Center.
    const auto center = place(model::BuildingType::CommandCenter, {0, 0});

    // Then exactly one construction drone is spawned for free.
    ASSERT_NE(center, 0U);
    ASSERT_EQ(spawns_.size(), 1U);
    EXPECT_EQ(spawns_.front().owner, kPlayer);
    EXPECT_EQ(spawns_.front().kind, events::DroneKind::Construction);
    EXPECT_EQ(droneCount(), 1);

    // When the same Command Center notification is seen again.
    bus_.publish(events::BuildingPlaced{center, kPlayer, model::BuildingType::CommandCenter, {-2, -2}, 5});

    // Then no extra free drone is granted.
    EXPECT_EQ(droneCount(), 1);
}

TEST_F(ConstructionDroneTest, RecruitCostsFiftyIronAndFiftyElectronicsFromTheCommandCenter)
{
    // Given a Command Center with the starting materials.
    const auto center = place(model::BuildingType::CommandCenter, {0, 0});
    ASSERT_NE(center, 0U);
    ASSERT_EQ(amountIn(center, model::ItemId::Iron), 500);
    ASSERT_EQ(amountIn(center, model::ItemId::Electronics), 100);

    // When the player recruits a construction drone.
    const bool recruited = drones_.recruit(kPlayer);

    // Then the cost is paid from the Command Center and a second drone appears.
    EXPECT_TRUE(recruited);
    EXPECT_EQ(amountIn(center, model::ItemId::Iron), 450);
    EXPECT_EQ(amountIn(center, model::ItemId::Electronics), 50);
    EXPECT_EQ(droneCount(), 2);
}

TEST_F(ConstructionDroneTest, RecruitFailsWithoutMaterials)
{
    // Given a Command Center without electronics.
    const auto center = place(model::BuildingType::CommandCenter, {0, 0});
    ASSERT_NE(center, 0U);
    auto* inventory = world_.tryGet<model::components::Inventory>(center);
    ASSERT_NE(inventory, nullptr);
    const int electronics = model::InventorySystem::amount(*inventory, model::ItemId::Electronics);
    EXPECT_EQ(model::InventorySystem::remove(*inventory, model::ItemId::Electronics, electronics), electronics);

    // When the player tries to recruit.
    const bool recruited = drones_.recruit(kPlayer);

    // Then nothing is paid and no extra drone is created.
    EXPECT_FALSE(recruited);
    EXPECT_EQ(amountIn(center, model::ItemId::Iron), 500);
    EXPECT_EQ(droneCount(), 1);
}

TEST_F(ConstructionDroneTest, RecruitIsFreeInDebugMode)
{
    // Given debug mode and a Command Center.
    bus_.publish(events::DebugToggleRequested{});
    const auto center = place(model::BuildingType::CommandCenter, {0, 0});
    ASSERT_NE(center, 0U);

    // When the player recruits.
    const bool recruited = drones_.recruit(kPlayer);

    // Then the drone is free.
    EXPECT_TRUE(recruited);
    EXPECT_EQ(amountIn(center, model::ItemId::Iron), 500);
    EXPECT_EQ(amountIn(center, model::ItemId::Electronics), 100);
    EXPECT_EQ(droneCount(), 2);
}

TEST_F(ConstructionDroneTest, CarriesAtMostTenUnitsPerTrip)
{
    // Given a stocked warehouse and a wall site far from it.
    (void)place(model::BuildingType::CommandCenter, {0, 0});
    const auto warehouse = place(model::BuildingType::Warehouse, {8, 8});
    ASSERT_NE(warehouse, 0U);
    stock(warehouse, model::ItemId::Iron, 100);
    const auto wall = startSite(model::BuildingType::Wall, {16, 16});

    // When the drone makes its first delivery.
    runUntil([&]() { return amountIn(wall, model::ItemId::Iron) > 0; }, 200.0F);

    // Then it carried at most one full load of ten units.
    EXPECT_EQ(amountIn(wall, model::ItemId::Iron), 10);
    EXPECT_EQ(amountIn(warehouse, model::ItemId::Iron), 90);
}

TEST_F(ConstructionDroneTest, PrefersTheNearestWarehouseOverTheCommandCenter)
{
    // Given a stocked warehouse near the site and the Command Center farther away.
    const auto center = place(model::BuildingType::CommandCenter, {0, 0});
    const auto warehouse = place(model::BuildingType::Warehouse, {10, 10});
    ASSERT_NE(center, 0U);
    ASSERT_NE(warehouse, 0U);
    stock(warehouse, model::ItemId::Iron, 100);
    const auto wall = startSite(model::BuildingType::Wall, {14, 14});

    // When the drone delivers the first load.
    runUntil([&]() { return amountIn(wall, model::ItemId::Iron) > 0; }, 200.0F);

    // Then it took the iron from the warehouse and left the Command Center untouched.
    EXPECT_EQ(amountIn(wall, model::ItemId::Iron), 10);
    EXPECT_EQ(amountIn(warehouse, model::ItemId::Iron), 90);
    EXPECT_EQ(amountIn(center, model::ItemId::Iron), 500);
}

TEST_F(ConstructionDroneTest, FallsBackToTheCommandCenterWhenNoWarehouseHasTheItem)
{
    // Given an empty warehouse and a stocked Command Center.
    const auto center = place(model::BuildingType::CommandCenter, {0, 0});
    const auto warehouse = place(model::BuildingType::Warehouse, {10, 10});
    ASSERT_NE(center, 0U);
    ASSERT_NE(warehouse, 0U);
    const auto wall = startSite(model::BuildingType::Wall, {14, 14});

    // When the drone delivers the first load.
    runUntil([&]() { return amountIn(wall, model::ItemId::Iron) > 0; }, 200.0F);

    // Then the iron came from the Command Center.
    EXPECT_EQ(amountIn(wall, model::ItemId::Iron), 10);
    EXPECT_EQ(amountIn(center, model::ItemId::Iron), 490);
}

TEST_F(ConstructionDroneTest, DeliversDifferentMaterialsToTheSameSite)
{
    // Given a warehouse with iron and coal and a wall site needing both.
    (void)place(model::BuildingType::CommandCenter, {0, 0});
    const auto warehouse = place(model::BuildingType::Warehouse, {8, 8});
    ASSERT_NE(warehouse, 0U);
    stock(warehouse, model::ItemId::Iron, 10);
    stock(warehouse, model::ItemId::Coal, 10);
    const auto wall = startSite(model::BuildingType::Wall, {12, 12});

    // When the drone runs until it has delivered both materials.
    runUntil([&]() {
        return amountIn(wall, model::ItemId::Iron) > 0 && amountIn(wall, model::ItemId::Coal) > 0;
    }, 200.0F);

    // Then both arrived at the site.
    EXPECT_EQ(amountIn(wall, model::ItemId::Iron), 10);
    EXPECT_EQ(amountIn(wall, model::ItemId::Coal), 10);
}

TEST_F(ConstructionDroneTest, DoesNotDeliverMaterialsInDebugMode)
{
    // Given debug mode, a stocked warehouse and a factory site.
    bus_.publish(events::DebugToggleRequested{});
    (void)place(model::BuildingType::CommandCenter, {0, 0});
    const auto warehouse = place(model::BuildingType::Warehouse, {8, 8});
    ASSERT_NE(warehouse, 0U);
    stock(warehouse, model::ItemId::Iron, 100);
    const auto factory = startSite(model::BuildingType::ToolFactory, {12, 12});

    // When the simulation runs long enough for the debug construction to finish.
    runUntil([]() { return false; }, 20.0F);

    // Then no material was taken from the warehouse or delivered to the site.
    EXPECT_EQ(amountIn(factory, model::ItemId::Iron), 0);
    EXPECT_EQ(amountIn(warehouse, model::ItemId::Iron), 100);
}
