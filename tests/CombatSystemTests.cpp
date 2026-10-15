#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/BuildingSystem.hpp"
#include "model/CombatSystem.hpp"
#include "model/components/BuildingComponents.hpp"
#include "model/UnitSystem.hpp"

#include <gtest/gtest.h>

namespace {

class CombatSystemTest : public ::testing::Test {
protected:
    CombatSystemTest()
        : buildings_(bus_, world_)
        , units_(bus_, world_, buildings_)
        , combat_(bus_, world_, units_, buildings_)
    {
    }

    core::EventBus bus_;
    model::ecs::Registry world_;
    model::BuildingSystem buildings_;
    model::UnitSystem units_;
    model::CombatSystem combat_;
};

} // namespace

TEST_F(CombatSystemTest, AttackOrderDamagesEnemyInRangeAfterCooldown)
{
    // Given opposing infantry units one tile apart.
    const auto attacker = units_.spawn(1, model::recruitStats(model::UnitType::Infantry), {0, 0});
    const auto target = units_.spawn(2, model::recruitStats(model::UnitType::Infantry), {1, 0});

    // When the attacker receives an order and its attack cooldown elapses.
    combat_.orderAttack(1, {attacker}, target);
    combat_.update(1.0F);

    // Then the enemy loses the attacker's damage.
    const auto* health = world_.tryGet<model::components::Health>(target);
    ASSERT_NE(health, nullptr);
    EXPECT_EQ(health->current, 90);
}

TEST_F(CombatSystemTest, RejectsFriendlyTargetsAndRetainsHealthOutsideRange)
{
    // Given an attacker and a distant enemy.
    const auto attacker = units_.spawn(1, model::recruitStats(model::UnitType::Infantry), {0, 0});
    const auto distant = units_.spawn(2, model::recruitStats(model::UnitType::Infantry), {8, 0});

    // When the attacker orders an invalid friendly target, then a distant target.
    combat_.orderAttack(1, {attacker}, attacker);
    combat_.orderAttack(1, {attacker}, distant);
    combat_.update(1.0F);

    // Then neither target is damaged.
    EXPECT_EQ(world_.tryGet<model::components::Health>(attacker)->current, 100);
    EXPECT_EQ(world_.tryGet<model::components::Health>(distant)->current, 100);
}

TEST_F(CombatSystemTest, TowerAutomaticallyAttacksNearestEnemyInRange)
{
    // Given a player with a Command Center and a defensive tower.
    buildings_.select(1, model::BuildingType::CommandCenter);
    buildings_.place(1, {0, 0});
    buildings_.select(1, model::BuildingType::Tower);
    buildings_.place(1, {10, 0});
    model::ecs::Entity tower = 0;
    world_.each<model::components::Building>([&](model::ecs::Entity entity, const model::components::Building& b) {
        if (b.type == model::BuildingType::Tower) tower = entity;
    });
    ASSERT_NE(tower, 0U);
    const auto enemy = units_.spawn(2, model::recruitStats(model::UnitType::Infantry), {11, 0});

    // When the combat simulation advances for one firing interval.
    combat_.update(1.0F);

    // Then the tower fires automatically and reduces the nearest enemy's health.
    const auto* health = world_.tryGet<model::components::Health>(enemy);
    ASSERT_NE(health, nullptr);
    EXPECT_EQ(health->current, 80);
}

TEST_F(CombatSystemTest, TowerAttacksTargetExactlyAtMaximumRange)
{
    // Given a tower with range six and an enemy exactly six tiles away.
    buildings_.select(1, model::BuildingType::CommandCenter);
    buildings_.place(1, {0, 0});
    buildings_.select(1, model::BuildingType::Tower);
    buildings_.place(1, {10, 0});
    model::ecs::Entity tower = 0;
    world_.each<model::components::Building>([&](model::ecs::Entity entity, const model::components::Building& b) {
        if (b.type == model::BuildingType::Tower) tower = entity;
    });
    ASSERT_NE(tower, 0U);
    const auto enemy = units_.spawn(2, model::recruitStats(model::UnitType::Infantry), {16, 0});
    world_.tryGet<model::components::Position>(enemy)->x = 16.0F;
    world_.tryGet<model::components::Position>(enemy)->y = 0.0F;

    // When one firing interval elapses.
    combat_.update(1.0F);

    // Then the boundary target is included in the tower's range.
    EXPECT_EQ(world_.tryGet<model::components::Health>(enemy)->current, 80);
}

TEST_F(CombatSystemTest, PublishesHealthChangeAfterDamage)
{
    // Given an attacker and an enemy, with health notifications observed.
    const auto attacker = units_.spawn(1, model::recruitStats(model::UnitType::Infantry), {0, 0});
    const auto target = units_.spawn(2, model::recruitStats(model::UnitType::Infantry), {1, 0});
    std::vector<events::HealthChanged> changes;
    bus_.subscribe<events::HealthChanged>([&](const auto& event) { changes.push_back(event); });

    // When the attacker deals damage.
    combat_.orderAttack(1, {attacker}, target);
    combat_.update(1.0F);

    // Then the model publishes the target's updated health.
    ASSERT_EQ(changes.size(), 1U);
    EXPECT_EQ(changes.front().entity, target);
    EXPECT_EQ(changes.front().health, 90);
    EXPECT_EQ(changes.front().maxHealth, 100);
}

