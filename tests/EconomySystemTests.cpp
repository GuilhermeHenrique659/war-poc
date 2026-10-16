#include "model/EconomySystem.hpp"
#include "model/UnitSystem.hpp"
#include "model/components/UnitComponents.hpp"

#include <gtest/gtest.h>

TEST(EconomySystemTests, GeneratesResourcesUpToCapacity)
{
    // Given an initialized player economy with a generator and finite storage.
    model::EconomySystem economy;
    economy.initializePlayer(7);
    economy.setCapacity(7, model::Resource::Gold, 25);
    economy.addGenerator(7, model::Resource::Gold, 10);

    // When one second elapses from the initial stock of 100.
    economy.update(1.0F);

    // Then the resource stock is capped at its capacity.
    EXPECT_EQ(economy.amount(7, model::Resource::Gold), 25);
}

TEST(EconomySystemTests, ChargesCostOnlyWhenAffordable)
{
    // Given an economy with enough gold but not enough steel.
    model::EconomySystem economy;
    economy.initializePlayer(3);
    const model::ResourceCost cost{20, 0, 50, 0};

    // When a purchase is attempted.
    EXPECT_FALSE(economy.spend(3, cost));

    // Then no resource is deducted.
    EXPECT_EQ(economy.amount(3, model::Resource::Gold), 100);
    EXPECT_EQ(economy.amount(3, model::Resource::Steel), 25);
}

TEST(EconomySystemTests, TracksPopulationCapacityAndUsage)
{
    // Given a player with base population capacity and no units.
    model::EconomySystem economy;
    economy.initializePlayer(2);
    economy.addPopulationCapacity(2, 5);

    // When four population is occupied.
    EXPECT_TRUE(economy.reservePopulation(2, 14));

    // Then one more fits but two do not.
    EXPECT_TRUE(economy.canReservePopulation(2, 1));
    EXPECT_FALSE(economy.canReservePopulation(2, 2));
}

TEST(UnitPopulationTests, RemovingUnitReleasesItsBasePopulation)
{
    // Given a unit representing three recruited units and three reserved population.
    core::EventBus bus;
    model::ecs::Registry world;
    model::BuildingSystem buildings(bus, world);
    model::UnitSystem units(bus, world, buildings);
    model::EconomySystem economy;
    economy.initializePlayer(2);
    economy.reservePopulation(2, 3);
    units.setEconomy(&economy);
    auto stats = model::recruitStats(model::UnitType::Soldier);
    stats.type = model::UnitType::Combined;
    stats.baseCount = 3;
    const auto entity = units.spawn(2, stats, {0, 0});

    // When the unit is removed.
    units.remove(entity);

    // Then all three represented population slots are available again.
    EXPECT_EQ(economy.populationUsed(2), 0);
}
