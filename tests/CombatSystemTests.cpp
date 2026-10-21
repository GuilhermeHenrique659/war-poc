#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/build/BuildingSystem.hpp"
#include "model/units/CombatSystem.hpp"
#include "model/player/PlayerAvatarSystem.hpp"
#include "model/build/BuildingComponents.hpp"
#include "model/economy/InventoryComponents.hpp"
#include "model/economy/InventorySystem.hpp"
#include "model/economy/Item.hpp"
#include "model/player/PlayerComponents.hpp"
#include "model/units/UnitComponents.hpp"
#include "model/units/UnitSystem.hpp"

#include <gtest/gtest.h>

namespace {

class CombatSystemTest : public ::testing::Test {
protected:
    CombatSystemTest()
        : buildings_(bus_, world_)
        , units_(bus_, world_, buildings_)
        , players_(bus_, world_)
        , combat_(bus_, world_, units_, buildings_)
    {
        const auto playerEntity = players_.create(1, "Tester");
        auto* inventory = world_.tryGet<model::components::Inventory>(playerEntity);
        EXPECT_NE(inventory, nullptr);
        if (inventory == nullptr) {
            return;
        }
        for (const auto item : {model::ItemId::Coal, model::ItemId::Steel, model::ItemId::Copper,
                 model::ItemId::Electronics, model::ItemId::Tools}) {
            EXPECT_EQ(model::InventorySystem::add(*inventory, item, 100), 100);
        }
    }

    core::EventBus bus_;
    model::ecs::Registry world_;
    model::BuildingSystem buildings_;
    model::UnitSystem units_;
    model::PlayerAvatarSystem players_;
    model::CombatSystem combat_;
};

} // namespace

TEST_F(CombatSystemTest, AttackOrderDamagesEnemyInRangeAfterCooldown)
{
    // Given opposing Soldier units one tile apart.
    const auto attacker = units_.spawn(1, model::recruitStats(model::UnitType::Soldier), {0, 0});
    const auto target = units_.spawn(2, model::recruitStats(model::UnitType::Soldier), {1, 0});

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
    const auto attacker = units_.spawn(1, model::recruitStats(model::UnitType::Soldier), {0, 0});
    const auto distant = units_.spawn(2, model::recruitStats(model::UnitType::Soldier), {8, 0});

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
    const auto enemy = units_.spawn(2, model::recruitStats(model::UnitType::Soldier), {11, 0});

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
    const auto enemy = units_.spawn(2, model::recruitStats(model::UnitType::Soldier), {16, 0});
    world_.tryGet<model::components::Position>(enemy)->x = 16.0F;
    world_.tryGet<model::components::Position>(enemy)->y = 0.0F;

    // When one firing interval elapses.
    combat_.update(1.0F);

    // Then the boundary target is included in the tower's range.
    EXPECT_EQ(world_.tryGet<model::components::Health>(enemy)->current, 80);
}

TEST_F(CombatSystemTest, TowerAutomaticallyAttacksAnEnemyPlayerAvatar)
{
    // Given a defensive tower and an enemy player avatar within its range.
    buildings_.select(1, model::BuildingType::CommandCenter);
    buildings_.place(1, {0, 0});
    buildings_.select(1, model::BuildingType::Tower);
    buildings_.place(1, {10, 0});
    const auto avatar = players_.create(2, "Inimigo");
    bus_.publish(events::BuildingPlaced{100, 2, model::BuildingType::CommandCenter, {9, -2}, 5});
    world_.tryGet<model::components::Position>(avatar)->x = 11.0F;
    world_.tryGet<model::components::Position>(avatar)->y = 0.0F;

    // When the tower advances by one firing interval.
    combat_.update(1.0F);

    // Then it damages the avatar, which respawns outside its Command Center.
    EXPECT_FLOAT_EQ(world_.tryGet<model::components::Position>(avatar)->x, 11.5F);
    EXPECT_FLOAT_EQ(world_.tryGet<model::components::Position>(avatar)->y, -2.5F);
    EXPECT_EQ(world_.tryGet<model::components::Health>(avatar)->current, 10);
}

TEST_F(CombatSystemTest, PublishesHealthChangeAfterDamage)
{
    // Given an attacker and an enemy, with health notifications observed.
    const auto attacker = units_.spawn(1, model::recruitStats(model::UnitType::Soldier), {0, 0});
    const auto target = units_.spawn(2, model::recruitStats(model::UnitType::Soldier), {1, 0});
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

TEST_F(CombatSystemTest, RespawnsAPlayerAvatarWhenCombatReducesItsHealthToZero)
{
    // Given an attacker beside a player avatar with a Command Center respawn point.
    const auto attacker = units_.spawn(1, model::recruitStats(model::UnitType::Soldier), {0, 0});
    const auto avatar = players_.create(2, "Jogador");
    bus_.publish(events::BuildingPlaced{99, 2, model::BuildingType::CommandCenter, {3, 4}, 5});
    world_.tryGet<model::components::Position>(avatar)->x = 1.5F;
    world_.tryGet<model::components::Position>(avatar)->y = 0.5F;
    std::vector<events::HealthChanged> changes;
    bus_.subscribe<events::HealthChanged>([&](const auto& event) { changes.push_back(event); });

    // When the attacker deals lethal damage.
    combat_.orderAttack(1, {attacker}, avatar);
    combat_.update(1.0F);

    // Then the avatar remains in the world, respawned at its starting point with full health.
    ASSERT_TRUE(world_.isAlive(avatar));
    EXPECT_FLOAT_EQ(world_.tryGet<model::components::Position>(avatar)->x, 5.5F);
    EXPECT_FLOAT_EQ(world_.tryGet<model::components::Position>(avatar)->y, 3.5F);
    EXPECT_EQ(world_.tryGet<model::components::Health>(avatar)->current, 10);
    ASSERT_EQ(changes.size(), 2U);
    EXPECT_EQ(changes.front().health, 0);
    EXPECT_EQ(changes.back().health, 10);
}
