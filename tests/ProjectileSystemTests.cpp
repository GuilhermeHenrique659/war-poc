#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/build/BuildingComponents.hpp"
#include "model/build/BuildingSystem.hpp"
#include "model/combat/DamageSystem.hpp"
#include "model/combat/ProjectileSystem.hpp"
#include "model/combat/WeaponComponents.hpp"
#include "model/units/UnitComponents.hpp"
#include "model/units/UnitSystem.hpp"
#include "model/units/UnitType.hpp"
#include "model/ecs/Registry.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <vector>

namespace {

class ProjectileSystemTest : public ::testing::Test {
protected:
    ProjectileSystemTest()
        : buildings_(bus_, world_)
        , units_(bus_, world_, buildings_)
        , damage_(bus_, world_, units_, buildings_)
        , projectiles_(bus_, world_, damage_)
    {
        // Free placement keeps the tests about ballistics, not economy.
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
    model::ProjectileSystem projectiles_;
};

} // namespace

TEST_F(ProjectileSystemTest, LaunchPublishesProjectileAndLocksTheImpactPoint)
{
    // Given a mortar shot aimed at an enemy.
    const auto source = units_.spawn(1, model::recruitStats(model::UnitType::Soldier), {0, 0});
    const auto target = units_.spawn(2, model::recruitStats(model::UnitType::Soldier), {5, 0});
    const model::components::Weapon mortar{model::WeaponType::Mortar, 15, 8.0F};
    std::vector<events::ProjectileSpawned> spawned;
    bus_.subscribe<events::ProjectileSpawned>([&](const auto& event) { spawned.push_back(event); });

    // When the shot is launched.
    const auto projectile
        = projectiles_.launch(1, source, target, mortar, 0.5F, 0.5F, 5.5F, 0.5F);

    // Then a projectile entity exists with the locked impact point and a velocity-based flight.
    ASSERT_TRUE(world_.isAlive(projectile));
    const auto* component = world_.tryGet<model::components::Projectile>(projectile);
    ASSERT_NE(component, nullptr);
    EXPECT_FLOAT_EQ(component->impactX, 5.5F);
    EXPECT_FLOAT_EQ(component->impactY, 0.5F);
    EXPECT_FLOAT_EQ(component->flightTime, 5.0F / 3.0F);
    ASSERT_EQ(spawned.size(), 1U);
    EXPECT_EQ(spawned.front().entity, projectile);
    EXPECT_EQ(spawned.front().target, target);
    EXPECT_EQ(spawned.front().weapon, model::WeaponType::Mortar);
    EXPECT_FLOAT_EQ(spawned.front().targetX, 5.5F);
}

TEST_F(ProjectileSystemTest, ProjectileFliesForItsFlightTimeBeforeDamaging)
{
    // Given a mortar shot six tiles away.
    const auto source = units_.spawn(1, model::recruitStats(model::UnitType::Soldier), {0, 0});
    const auto target = units_.spawn(2, model::recruitStats(model::UnitType::Soldier), {6, 0});
    const model::components::Weapon mortar{model::WeaponType::Mortar, 15, 8.0F};
    const auto projectile
        = projectiles_.launch(1, source, target, mortar, 0.5F, 0.5F, 6.5F, 0.5F);

    // When half of the flight time passes.
    projectiles_.update(1.0F);

    // Then the target is still unharmed and the projectile is in the air.
    EXPECT_EQ(healthOf(target), 100);
    ASSERT_TRUE(world_.isAlive(projectile));

    // When the projectile reaches the impact point.
    projectiles_.update(1.0F);

    // Then it damages the target and is removed.
    EXPECT_EQ(healthOf(target), 85);
    EXPECT_FALSE(world_.isAlive(projectile));
}

TEST_F(ProjectileSystemTest, ExplosionHitsFriendliesInsideTheArea)
{
    // Given an ally and an enemy near the impact point.
    const auto source = units_.spawn(1, model::recruitStats(model::UnitType::Soldier), {0, 0});
    const auto ally = units_.spawn(1, model::recruitStats(model::UnitType::Soldier), {2, 0});
    const auto enemy = units_.spawn(2, model::recruitStats(model::UnitType::Soldier), {3, 0});
    const model::components::Weapon mortar{model::WeaponType::Mortar, 15, 8.0F};
    (void)projectiles_.launch(1, source, enemy, mortar, 0.5F, 0.5F, 3.5F, 0.5F);

    // When the shell explodes.
    projectiles_.update(1.0F);

    // Then ally and enemy take damage, while the shooter outside the radius is spared.
    EXPECT_EQ(healthOf(ally), 85);
    EXPECT_EQ(healthOf(enemy), 85);
    EXPECT_EQ(healthOf(source), 100);
}

TEST_F(ProjectileSystemTest, ProjectileIgnoresBuildingsInItsPath)
{
    // Given a wall between the mortar and its target.
    buildings_.select(1, model::BuildingType::CommandCenter);
    buildings_.place(1, {20, 20});
    buildings_.select(1, model::BuildingType::Wall);
    buildings_.place(1, {3, 0});
    const auto wall = buildingOfType(model::BuildingType::Wall);
    ASSERT_NE(wall, 0U);
    const auto source = units_.spawn(1, model::recruitStats(model::UnitType::Soldier), {0, 0});
    const auto target = units_.spawn(2, model::recruitStats(model::UnitType::Soldier), {6, 0});
    const model::components::Weapon mortar{model::WeaponType::Mortar, 15, 8.0F};
    (void)projectiles_.launch(1, source, target, mortar, 0.5F, 0.5F, 6.5F, 0.5F);

    // When the shell lands beyond the wall.
    projectiles_.update(2.0F);

    // Then the target is hit and the wall outside the blast radius is untouched.
    EXPECT_EQ(healthOf(target), 85);
    EXPECT_EQ(healthOf(wall), 500);
}

TEST_F(ProjectileSystemTest, ResetRemovesEveryProjectileInFlight)
{
    // Given a projectile in the air.
    const auto source = units_.spawn(1, model::recruitStats(model::UnitType::Soldier), {0, 0});
    const auto target = units_.spawn(2, model::recruitStats(model::UnitType::Soldier), {6, 0});
    const model::components::Weapon mortar{model::WeaponType::Mortar, 15, 8.0F};
    const auto projectile
        = projectiles_.launch(1, source, target, mortar, 0.5F, 0.5F, 6.5F, 0.5F);
    projectiles_.update(0.5F);
    ASSERT_TRUE(world_.isAlive(projectile));

    // When the system is reset.
    projectiles_.reset();

    // Then the projectile is gone.
    EXPECT_FALSE(world_.isAlive(projectile));
}
