#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/build/BuildingComponents.hpp"
#include "model/build/BuildingSystem.hpp"
#include "model/combat/DamageSystem.hpp"
#include "model/units/UnitComponents.hpp"
#include "model/units/UnitSystem.hpp"
#include "model/units/UnitType.hpp"
#include "model/ecs/Registry.hpp"

#include <gtest/gtest.h>

#include <vector>

namespace {

class DamageSystemTest : public ::testing::Test {
protected:
    DamageSystemTest()
        : buildings_(bus_, world_)
        , units_(bus_, world_, buildings_)
        , damage_(bus_, world_, units_, buildings_)
    {
        // Free placement keeps the tests about damage, not economy.
        bus_.publish(events::DebugToggleRequested{});
    }

    [[nodiscard]] model::ecs::Entity buildingOfType(model::BuildingType type)
    {
        model::ecs::Entity found = 0;
        world_.each<model::components::Building>([&](model::ecs::Entity entity, const auto& building) {
            if (building.type == type) {
                found = entity;
            }
        });
        return found;
    }

    [[nodiscard]] int healthOf(model::ecs::Entity entity)
    {
        const auto* health = world_.tryGet<model::components::Health>(entity);
        return health == nullptr ? -1 : health->current;
    }

    core::EventBus bus_;
    model::ecs::Registry world_;
    model::BuildingSystem buildings_;
    model::UnitSystem units_;
    model::DamageSystem damage_;
};

} // namespace

TEST_F(DamageSystemTest, KineticDamageHitsOnlyTheTarget)
{
    // Given two enemy soldiers standing side by side.
    const auto target = units_.spawn(1, model::recruitStats(model::UnitType::Soldier), {0, 0});
    const auto bystander = units_.spawn(2, model::recruitStats(model::UnitType::Soldier), {1, 0});

    // When one of them takes kinetic damage.
    const bool defeated = damage_.apply(target, model::DamageSpec{model::DamageType::Kinetic, 10, 0.0F});

    // Then only the target loses health.
    EXPECT_FALSE(defeated);
    EXPECT_EQ(healthOf(target), 90);
    EXPECT_EQ(healthOf(bystander), 100);
}

TEST_F(DamageSystemTest, ExplosiveDamageHitsEveryEntityInTheRadiusIncludingAllies)
{
    // Given an ally, an enemy and a distant unit around an impact point.
    const auto ally = units_.spawn(1, model::recruitStats(model::UnitType::Soldier), {0, 0});
    const auto enemy = units_.spawn(2, model::recruitStats(model::UnitType::Soldier), {1, 0});
    const auto distant = units_.spawn(2, model::recruitStats(model::UnitType::Soldier), {5, 0});

    // When an explosive lands next to them.
    const std::size_t hit = damage_.applyArea(1.0F, 0.5F, model::DamageSpec{model::DamageType::Explosive, 30, 2.0F});

    // Then ally and enemy are both damaged, and the distant unit is untouched.
    EXPECT_EQ(hit, 2U);
    EXPECT_EQ(healthOf(ally), 70);
    EXPECT_EQ(healthOf(enemy), 70);
    EXPECT_EQ(healthOf(distant), 100);
}

TEST_F(DamageSystemTest, ExplosiveDamageAlsoHitsFriendlyBuildings)
{
    // Given a player's own wall next to an impact point.
    buildings_.select(1, model::BuildingType::CommandCenter);
    buildings_.place(1, {20, 20});
    buildings_.select(1, model::BuildingType::Wall);
    buildings_.place(1, {2, 0});
    const auto wall = buildingOfType(model::BuildingType::Wall);
    ASSERT_NE(wall, 0U);

    // When an explosion covers the wall.
    const std::size_t hit = damage_.applyArea(2.5F, 0.5F, model::DamageSpec{model::DamageType::Explosive, 30, 2.0F});

    // Then the wall takes the damage.
    EXPECT_EQ(hit, 1U);
    EXPECT_EQ(healthOf(wall), 470);
}

TEST_F(DamageSystemTest, PublishesHealthChangedForEachDamagedEntity)
{
    // Given two enemies inside an explosion.
    const auto first = units_.spawn(1, model::recruitStats(model::UnitType::Soldier), {0, 0});
    const auto second = units_.spawn(2, model::recruitStats(model::UnitType::Soldier), {1, 0});
    std::vector<events::HealthChanged> changes;
    bus_.subscribe<events::HealthChanged>([&](const auto& event) { changes.push_back(event); });

    // When the explosion lands.
    (void)damage_.applyArea(1.0F, 0.5F, model::DamageSpec{model::DamageType::Explosive, 25, 2.0F});

    // Then each damaged entity gets its own notification (in ECS iteration order).
    ASSERT_EQ(changes.size(), 2U);
    for (const events::HealthChanged& change : changes) {
        EXPECT_EQ(change.health, 75);
        EXPECT_EQ(change.maxHealth, 100);
        EXPECT_TRUE(change.entity == first || change.entity == second);
    }
}

TEST_F(DamageSystemTest, DefeatingAUnitRemovesItFromTheWorld)
{
    // Given a soldier one hit away from death.
    const auto unit = units_.spawn(1, model::recruitStats(model::UnitType::Soldier), {0, 0});
    world_.tryGet<model::components::Health>(unit)->current = 5;

    // When it takes lethal kinetic damage.
    const bool defeated = damage_.apply(unit, model::DamageSpec{model::DamageType::Kinetic, 10, 0.0F});

    // Then it is removed from the world.
    EXPECT_TRUE(defeated);
    EXPECT_FALSE(world_.isAlive(unit));
}

TEST_F(DamageSystemTest, DefeatingABuildingRemovesItAndFreesItsTiles)
{
    // Given a training dummy one hit away from death.
    buildings_.select(1, model::BuildingType::CommandCenter);
    buildings_.place(1, {20, 20});
    buildings_.select(1, model::BuildingType::TrainingDummy);
    buildings_.place(1, {5, 5});
    const auto dummy = buildingOfType(model::BuildingType::TrainingDummy);
    ASSERT_NE(dummy, 0U);
    world_.tryGet<model::components::Health>(dummy)->current = 5;

    // When it takes lethal kinetic damage.
    const bool defeated = damage_.apply(dummy, model::DamageSpec{model::DamageType::Kinetic, 10, 0.0F});

    // Then it is removed and its tile is free again.
    EXPECT_TRUE(defeated);
    EXPECT_FALSE(world_.isAlive(dummy));
    EXPECT_FALSE(buildings_.isOccupied({5, 5}));
}
