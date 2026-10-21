#include "model/economy/InventoryComponents.hpp"
#include "model/economy/InventorySystem.hpp"
#include "model/economy/ProductionComponents.hpp"
#include "model/economy/ProductionSystem.hpp"
#include "model/ecs/Registry.hpp"

#include <gtest/gtest.h>

TEST(ProductionSystemTests, PrimaryProducerGeneratesOnlyWholeOutputUnits)
{
    // Given a mine with an empty inventory and production of one iron per second.
    model::ecs::Registry world;
    const auto mine = world.create();
    world.emplace<model::components::Inventory>(mine, std::size_t{2});
    const auto* inventory = world.tryGet<model::components::Inventory>(mine);
    world.emplace<model::components::Production>(
        mine, std::vector<model::ProductionInput>{}, model::ItemId::Iron, 1.0F);
    model::ProductionSystem production;

    // When two half-second updates pass.
    production.update(world, 0.5F);
    ASSERT_NE(inventory, nullptr);
    EXPECT_EQ(model::InventorySystem::amount(*inventory, model::ItemId::Iron), 0);
    production.update(world, 0.5F);

    // Then one whole iron is stored.
    EXPECT_EQ(model::InventorySystem::amount(*inventory, model::ItemId::Iron), 1);
}

TEST(ProductionSystemTests, IntermediateProducerConsumesWholeInputsAndAccumulatesOutput)
{
    // Given a steelworks with enough iron and coal, producing half a steel per second.
    model::ecs::Registry world;
    const auto steelworks = world.create();
    world.emplace<model::components::Inventory>(steelworks, std::size_t{4});
    auto* inventory = world.tryGet<model::components::Inventory>(steelworks);
    model::InventorySystem::add(*inventory, model::ItemId::Iron, 100);
    model::InventorySystem::add(*inventory, model::ItemId::Coal, 100);
    world.emplace<model::components::Production>(
        steelworks,
        std::vector<model::ProductionInput>{{model::ItemId::Iron, 1.0F}, {model::ItemId::Coal, 1.0F}},
        model::ItemId::Steel,
        0.5F);
    model::ProductionSystem production;

    // When two seconds elapse.
    production.update(world, 1.0F);
    ASSERT_NE(inventory, nullptr);
    EXPECT_EQ(model::InventorySystem::amount(*inventory, model::ItemId::Steel), 0);
    production.update(world, 1.0F);

    // Then it consumes two whole units of each input and stores one whole steel.
    EXPECT_EQ(model::InventorySystem::amount(*inventory, model::ItemId::Iron), 98);
    EXPECT_EQ(model::InventorySystem::amount(*inventory, model::ItemId::Coal), 98);
    EXPECT_EQ(model::InventorySystem::amount(*inventory, model::ItemId::Steel), 1);
}

TEST(ProductionSystemTests, MissingInputStopsProductionWithoutConsumingOtherInputs)
{
    // Given a steelworks with iron but no coal.
    model::ecs::Registry world;
    const auto steelworks = world.create();
    world.emplace<model::components::Inventory>(steelworks, std::size_t{3});
    auto* inventory = world.tryGet<model::components::Inventory>(steelworks);
    model::InventorySystem::add(*inventory, model::ItemId::Iron, 100);
    world.emplace<model::components::Production>(
        steelworks,
        std::vector<model::ProductionInput>{{model::ItemId::Iron, 1.0F}, {model::ItemId::Coal, 1.0F}},
        model::ItemId::Steel,
        0.5F);
    model::ProductionSystem production;

    // When one second elapses.
    production.update(world, 1.0F);

    // Then no input is consumed and no steel is produced.
    ASSERT_NE(inventory, nullptr);
    EXPECT_EQ(model::InventorySystem::amount(*inventory, model::ItemId::Iron), 100);
    EXPECT_EQ(model::InventorySystem::amount(*inventory, model::ItemId::Coal), 0);
    EXPECT_EQ(model::InventorySystem::amount(*inventory, model::ItemId::Steel), 0);
}

TEST(ProductionSystemTests, LimitsProductionToAvailableInput)
{
    // Given a steelworks with one iron and 100 coal.
    model::ecs::Registry world;
    const auto steelworks = world.create();
    world.emplace<model::components::Inventory>(steelworks, std::size_t{3});
    auto* inventory = world.tryGet<model::components::Inventory>(steelworks);
    model::InventorySystem::add(*inventory, model::ItemId::Iron, 1);
    model::InventorySystem::add(*inventory, model::ItemId::Coal, 100);
    world.emplace<model::components::Production>(
        steelworks,
        std::vector<model::ProductionInput>{{model::ItemId::Iron, 1.0F}, {model::ItemId::Coal, 1.0F}},
        model::ItemId::Steel,
        0.5F);
    model::ProductionSystem production;

    // When one second elapses.
    production.update(world, 1.0F);

    // Then it consumes one iron and coal but stores no fractional steel.
    ASSERT_NE(inventory, nullptr);
    EXPECT_EQ(model::InventorySystem::amount(*inventory, model::ItemId::Iron), 0);
    EXPECT_EQ(model::InventorySystem::amount(*inventory, model::ItemId::Coal), 99);
    EXPECT_EQ(model::InventorySystem::amount(*inventory, model::ItemId::Steel), 0);
}

TEST(ProductionSystemTests, StopsWhenOutputInventoryIsFull)
{
    // Given a primary producer whose one-slot inventory is full.
    model::ecs::Registry world;
    const auto mine = world.create();
    world.emplace<model::components::Inventory>(mine, std::size_t{1});
    auto* inventory = world.tryGet<model::components::Inventory>(mine);
    model::InventorySystem::add(*inventory, model::ItemId::Iron, 100);
    world.emplace<model::components::Production>(
        mine, std::vector<model::ProductionInput>{}, model::ItemId::Iron, 1.0F);
    model::ProductionSystem production;

    // When one second elapses.
    production.update(world, 1.0F);

    // Then inventory remains at capacity.
    ASSERT_NE(inventory, nullptr);
    EXPECT_EQ(model::InventorySystem::amount(*inventory, model::ItemId::Iron), 100);
}
