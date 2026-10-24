#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/build/BuildingSystem.hpp"
#include "model/combat/DamageSystem.hpp"
#include "model/combat/ProjectileSystem.hpp"
#include "model/combat/WeaponComponents.hpp"
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
        , damage_(bus_, world_, units_, buildings_)
        , projectiles_(bus_, world_, damage_)
        , combat_(bus_, world_, units_, buildings_, damage_, projectiles_)
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

    void equip(model::ecs::Entity unit, int ammo = model::kUnitAmmoCapacity)
    {
        auto* inventory = world_.tryGet<model::components::Inventory>(unit);
        ASSERT_NE(inventory, nullptr);
        EXPECT_EQ(model::InventorySystem::add(*inventory, model::ItemId::Weapon, model::kUnitWeaponCount),
            model::kUnitWeaponCount);
        if (ammo > 0) {
            EXPECT_EQ(model::InventorySystem::add(*inventory, model::ItemId::Ammo, ammo), ammo);
        }
    }

    core::EventBus bus_;
    model::ecs::Registry world_;
    model::BuildingSystem buildings_;
    model::UnitSystem units_;
    model::PlayerAvatarSystem players_;
    model::DamageSystem damage_;
    model::ProjectileSystem projectiles_;
    model::CombatSystem combat_;
};

} // namespace

TEST_F(CombatSystemTest, AttackOrderDamagesEnemyInRangeAfterCooldown)
{
    // Given opposing Soldier units one tile apart.
    const auto attacker = units_.spawn(1, model::recruitStats(model::UnitType::Soldier), {0, 0});
    const auto target = units_.spawn(2, model::recruitStats(model::UnitType::Soldier), {1, 0});
    equip(attacker);

    // When the attacker receives an order and its attack cooldown elapses.
    combat_.orderAttack(1, {attacker}, target);
    combat_.update(1.0F);

    // Then the enemy loses the attacker's damage.
    const auto* health = world_.tryGet<model::components::Health>(target);
    ASSERT_NE(health, nullptr);
    EXPECT_EQ(health->current, 90);
}

TEST_F(CombatSystemTest, RifleHitsATargetAtItsMaximumRangeOfFourTiles)
{
    // Given a soldier and an enemy exactly four tiles away.
    const auto attacker = units_.spawn(1, model::recruitStats(model::UnitType::Soldier), {0, 0});
    const auto target = units_.spawn(2, model::recruitStats(model::UnitType::Soldier), {4, 0});
    equip(attacker);

    // When the soldier fires.
    combat_.orderAttack(1, {attacker}, target);
    combat_.update(1.0F);

    // Then the target at the rifle's range boundary takes damage.
    EXPECT_EQ(world_.tryGet<model::components::Health>(target)->current, 90);
}

TEST_F(CombatSystemTest, RejectsFriendlyTargetsAndRetainsHealthOutsideRange)
{
    // Given an attacker and a distant enemy.
    const auto attacker = units_.spawn(1, model::recruitStats(model::UnitType::Soldier), {0, 0});
    const auto distant = units_.spawn(2, model::recruitStats(model::UnitType::Soldier), {8, 0});
    equip(attacker);

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
    equip(attacker);
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

TEST_F(CombatSystemTest, DirectShotPublishesShotFiredForTheView)
{
    // Given an armed soldier and an enemy two tiles away, with shot events observed.
    const auto attacker = units_.spawn(1, model::recruitStats(model::UnitType::Soldier), {0, 0});
    const auto target = units_.spawn(2, model::recruitStats(model::UnitType::Soldier), {2, 0});
    equip(attacker);
    std::vector<events::ShotFired> shots;
    bus_.subscribe<events::ShotFired>([&](const auto& event) { shots.push_back(event); });

    // When the soldier fires.
    combat_.orderAttack(1, {attacker}, target);
    combat_.update(1.0F);

    // Then the view receives both endpoints and the weapon that fired.
    ASSERT_EQ(shots.size(), 1U);
    EXPECT_EQ(shots.front().source, attacker);
    EXPECT_EQ(shots.front().target, target);
    EXPECT_EQ(shots.front().owner, 1);
    EXPECT_EQ(shots.front().weapon, model::WeaponType::Rifle);
    EXPECT_FLOAT_EQ(shots.front().sourceX, 0.5F);
    EXPECT_FLOAT_EQ(shots.front().sourceY, 0.5F);
    EXPECT_FLOAT_EQ(shots.front().targetX, 2.5F);
    EXPECT_FLOAT_EQ(shots.front().targetY, 0.5F);
}

TEST_F(CombatSystemTest, HeldFirePublishesNoShotEffect)
{
    // Given a wall blocking the line of sight, with shot events observed.
    buildings_.select(1, model::BuildingType::CommandCenter);
    buildings_.place(1, {20, 20});
    buildings_.select(1, model::BuildingType::Wall);
    buildings_.place(1, {1, 0});
    const auto attacker = units_.spawn(1, model::recruitStats(model::UnitType::Soldier), {0, 0});
    const auto target = units_.spawn(2, model::recruitStats(model::UnitType::Soldier), {2, 0});
    equip(attacker);
    std::vector<events::ShotFired> shots;
    bus_.subscribe<events::ShotFired>([&](const auto& event) { shots.push_back(event); });

    // When the soldier holds fire.
    combat_.orderAttack(1, {attacker}, target);
    combat_.update(1.0F);

    // Then no tracer is announced.
    EXPECT_TRUE(shots.empty());
}

TEST_F(CombatSystemTest, ParabolicShotDoesNotPublishTheDirectTracer)
{
    // Given a mortar soldier and a distant target, with shot events observed.
    model::UnitStats mortarStats = model::recruitStats(model::UnitType::Soldier);
    mortarStats.weapon = model::WeaponType::Mortar;
    mortarStats.damage = model::weaponSpec(model::WeaponType::Mortar).damage;
    mortarStats.range = model::weaponSpec(model::WeaponType::Mortar).range;
    const auto attacker = units_.spawn(1, mortarStats, {0, 0});
    const auto target = units_.spawn(2, model::recruitStats(model::UnitType::Soldier), {6, 0});
    equip(attacker);
    std::vector<events::ShotFired> shots;
    bus_.subscribe<events::ShotFired>([&](const auto& event) { shots.push_back(event); });

    // When the mortar fires.
    combat_.orderAttack(1, {attacker}, target);
    combat_.update(1.0F);

    // Then only the projectile event is used, not the direct tracer.
    EXPECT_TRUE(shots.empty());
}

TEST_F(CombatSystemTest, RespawnsAPlayerAvatarWhenCombatReducesItsHealthToZero)
{
    // Given an attacker beside a player avatar with a Command Center respawn point.
    const auto attacker = units_.spawn(1, model::recruitStats(model::UnitType::Soldier), {0, 0});
    equip(attacker);
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

TEST_F(CombatSystemTest, EachAttackConsumesOneAmmoAndStopsFiringWhenAmmoRunsOut)
{
    // Given an attacker with a weapon and only two rounds of ammo.
    const auto attacker = units_.spawn(1, model::recruitStats(model::UnitType::Soldier), {0, 0});
    const auto target = units_.spawn(2, model::recruitStats(model::UnitType::Soldier), {1, 0});
    equip(attacker, 2);

    // When it attacks for three firing intervals.
    combat_.orderAttack(1, {attacker}, target);
    combat_.update(1.0F);
    auto* inventory = world_.tryGet<model::components::Inventory>(attacker);
    ASSERT_NE(inventory, nullptr);
    EXPECT_EQ(model::InventorySystem::amount(*inventory, model::ItemId::Ammo), 1);
    EXPECT_EQ(world_.tryGet<model::components::Health>(target)->current, 90);
    combat_.update(1.0F);
    EXPECT_EQ(model::InventorySystem::amount(*inventory, model::ItemId::Ammo), 0);
    EXPECT_EQ(world_.tryGet<model::components::Health>(target)->current, 80);
    combat_.update(1.0F);

    // Then the third interval fires nothing because the unit has no ammo left.
    EXPECT_EQ(model::InventorySystem::amount(*inventory, model::ItemId::Ammo), 0);
    EXPECT_EQ(world_.tryGet<model::components::Health>(target)->current, 80);
}

TEST_F(CombatSystemTest, UnitsCanAttackThePlayersOwnTrainingDummy)
{
    // Given a player's soldier and a Training Dummy one tile away.
    buildings_.select(1, model::BuildingType::CommandCenter);
    buildings_.place(1, {0, 0});
    buildings_.select(1, model::BuildingType::TrainingDummy);
    buildings_.place(1, {5, 5});
    const model::ecs::Entity dummy = buildingOfType(model::BuildingType::TrainingDummy);
    ASSERT_NE(dummy, 0U);
    const auto attacker = units_.spawn(1, model::recruitStats(model::UnitType::Soldier), {4, 5});
    equip(attacker);

    // When the soldier receives an order to attack the friendly dummy.
    combat_.orderAttack(1, {attacker}, dummy);
    combat_.update(1.0F);

    // Then the dummy takes damage like an enemy target.
    const auto* health = world_.tryGet<model::components::Health>(dummy);
    ASSERT_NE(health, nullptr);
    EXPECT_EQ(health->current, 90);
}

TEST_F(CombatSystemTest, DestroyingATrainingDummyRemovesItAndFreesItsTile)
{
    // Given a Training Dummy with little health left and an armed soldier beside it.
    buildings_.select(1, model::BuildingType::CommandCenter);
    buildings_.place(1, {0, 0});
    buildings_.select(1, model::BuildingType::TrainingDummy);
    buildings_.place(1, {5, 5});
    const model::ecs::Entity dummy = buildingOfType(model::BuildingType::TrainingDummy);
    ASSERT_NE(dummy, 0U);
    world_.tryGet<model::components::Health>(dummy)->current = 10;
    const auto attacker = units_.spawn(1, model::recruitStats(model::UnitType::Soldier), {4, 5});
    equip(attacker);

    // When the soldier deals lethal damage.
    combat_.orderAttack(1, {attacker}, dummy);
    combat_.update(1.0F);

    // Then the dummy is removed from the world and its tile becomes free.
    EXPECT_FALSE(world_.isAlive(dummy));
    EXPECT_FALSE(buildings_.isOccupied({5, 5}));
}

TEST_F(CombatSystemTest, AttackOrderWalksUntilTheTargetEntersWeaponRange)
{
    // Given a soldier far from the player's Training Dummy.
    buildings_.select(1, model::BuildingType::CommandCenter);
    buildings_.place(1, {0, 0});
    buildings_.select(1, model::BuildingType::TrainingDummy);
    buildings_.place(1, {10, 10});
    const model::ecs::Entity dummy = buildingOfType(model::BuildingType::TrainingDummy);
    ASSERT_NE(dummy, 0U);
    const auto attacker = units_.spawn(1, model::recruitStats(model::UnitType::Soldier), {0, 10});
    equip(attacker);
    const float range = model::weaponSpec(model::WeaponType::Rifle).range;

    // When the attack order and time pass.
    combat_.orderAttack(1, {attacker}, dummy);
    for (int step = 0; step < 200; ++step) {
        combat_.update(0.05F);
        units_.update(0.05F);
    }

    // Then it stopped as soon as the dummy was inside the rifle range, without hugging it.
    const auto* position = world_.tryGet<model::components::Position>(attacker);
    ASSERT_NE(position, nullptr);
    const float dx = position->x - 10.5F;
    const float dy = position->y - 10.5F;
    EXPECT_GT(dx * dx + dy * dy, 1.0F);
    EXPECT_LE(dx * dx + dy * dy, range * range + 0.01F);
    const auto* health = world_.tryGet<model::components::Health>(dummy);
    ASSERT_NE(health, nullptr);
    EXPECT_LT(health->current, 100);
}

TEST_F(CombatSystemTest, AttackOrderKeepsAnInRangeSoldierInPlaceAndFires)
{
    // Given a soldier that is already three tiles from the dummy, inside the rifle range.
    buildings_.select(1, model::BuildingType::CommandCenter);
    buildings_.place(1, {0, 0});
    buildings_.select(1, model::BuildingType::TrainingDummy);
    buildings_.place(1, {10, 10});
    const model::ecs::Entity dummy = buildingOfType(model::BuildingType::TrainingDummy);
    ASSERT_NE(dummy, 0U);
    const auto attacker = units_.spawn(1, model::recruitStats(model::UnitType::Soldier), {7, 10});
    equip(attacker);
    const auto* position = world_.tryGet<model::components::Position>(attacker);
    ASSERT_NE(position, nullptr);
    const float startX = position->x;
    const float startY = position->y;

    // When it receives the attack order and time passes.
    combat_.orderAttack(1, {attacker}, dummy);
    for (int step = 0; step < 100; ++step) {
        combat_.update(0.05F);
        units_.update(0.05F);
    }

    // Then it stays where it was and shoots from the range it already had.
    EXPECT_FLOAT_EQ(position->x, startX);
    EXPECT_FLOAT_EQ(position->y, startY);
    EXPECT_LT(world_.tryGet<model::components::Health>(dummy)->current, 100);
}

TEST_F(CombatSystemTest, DoesNotAttackWithoutAWeapon)
{
    // Given an attacker with ammo but no weapon.
    const auto attacker = units_.spawn(1, model::recruitStats(model::UnitType::Soldier), {0, 0});
    const auto target = units_.spawn(2, model::recruitStats(model::UnitType::Soldier), {1, 0});
    auto* inventory = world_.tryGet<model::components::Inventory>(attacker);
    ASSERT_NE(inventory, nullptr);
    EXPECT_EQ(model::InventorySystem::add(*inventory, model::ItemId::Ammo, 100), 100);

    // When it receives an attack order and the cooldown elapses.
    combat_.orderAttack(1, {attacker}, target);
    combat_.update(1.0F);

    // Then it cannot deal damage and does not spend ammo.
    EXPECT_EQ(world_.tryGet<model::components::Health>(target)->current, 100);
    EXPECT_EQ(model::InventorySystem::amount(*inventory, model::ItemId::Ammo), 100);
}

TEST_F(CombatSystemTest, DirectFireWithoutLineOfSightHoldsFireWithoutSpendingAmmo)
{
    // Given a wall between an armed soldier and its target.
    buildings_.select(1, model::BuildingType::CommandCenter);
    buildings_.place(1, {20, 20});
    buildings_.select(1, model::BuildingType::Wall);
    buildings_.place(1, {1, 0});
    const model::ecs::Entity wall = buildingOfType(model::BuildingType::Wall);
    ASSERT_NE(wall, 0U);
    const auto attacker = units_.spawn(1, model::recruitStats(model::UnitType::Soldier), {0, 0});
    const auto target = units_.spawn(2, model::recruitStats(model::UnitType::Soldier), {2, 0});
    equip(attacker);

    // When the soldier fires while the wall blocks the line of sight.
    combat_.orderAttack(1, {attacker}, target);
    combat_.update(1.0F);

    // Then it holds fire and keeps its ammo.
    EXPECT_EQ(world_.tryGet<model::components::Health>(target)->current, 100);
    EXPECT_EQ(model::InventorySystem::amount(*world_.tryGet<model::components::Inventory>(attacker),
                  model::ItemId::Ammo),
        model::kUnitAmmoCapacity);

    // When the wall is removed and the soldier fires again.
    buildings_.remove(wall);
    combat_.update(1.0F);

    // Then the shot lands.
    EXPECT_EQ(world_.tryGet<model::components::Health>(target)->current, 90);
    EXPECT_EQ(model::InventorySystem::amount(*world_.tryGet<model::components::Inventory>(attacker),
                  model::ItemId::Ammo),
        model::kUnitAmmoCapacity - 1);
}

TEST_F(CombatSystemTest, TerrainBlockedTilesStopDirectFire)
{
    // Given a custom sight blocker between an armed soldier and its target.
    const auto attacker = units_.spawn(1, model::recruitStats(model::UnitType::Soldier), {0, 0});
    const auto target = units_.spawn(2, model::recruitStats(model::UnitType::Soldier), {2, 0});
    equip(attacker);
    combat_.setTileSightBlocker([](model::GridPos tile) { return tile == model::GridPos{1, 0}; });

    // When the soldier tries to fire through it.
    combat_.orderAttack(1, {attacker}, target);
    combat_.update(1.0F);

    // Then it holds fire.
    EXPECT_EQ(world_.tryGet<model::components::Health>(target)->current, 100);
    EXPECT_EQ(model::InventorySystem::amount(*world_.tryGet<model::components::Inventory>(attacker),
                  model::ItemId::Ammo),
        model::kUnitAmmoCapacity);
}

TEST_F(CombatSystemTest, ParabolicFireIgnoresObstaclesAndDamagesOnImpact)
{
    // Given a mortar soldier and a wall blocking the line to its target.
    buildings_.select(1, model::BuildingType::CommandCenter);
    buildings_.place(1, {20, 20});
    buildings_.select(1, model::BuildingType::Wall);
    buildings_.place(1, {3, 0});
    model::UnitStats mortarStats = model::recruitStats(model::UnitType::Soldier);
    mortarStats.weapon = model::WeaponType::Mortar;
    mortarStats.damage = model::weaponSpec(model::WeaponType::Mortar).damage;
    mortarStats.range = model::weaponSpec(model::WeaponType::Mortar).range;
    const auto attacker = units_.spawn(1, mortarStats, {0, 0});
    const auto target = units_.spawn(2, model::recruitStats(model::UnitType::Soldier), {6, 0});
    equip(attacker);

    // When the mortar fires.
    combat_.orderAttack(1, {attacker}, target);
    combat_.update(1.0F);

    // Then the shell is in flight and the target is not hurt yet.
    EXPECT_EQ(world_.tryGet<model::components::Health>(target)->current, 100);
    EXPECT_EQ(model::InventorySystem::amount(*world_.tryGet<model::components::Inventory>(attacker),
                  model::ItemId::Ammo),
        model::kUnitAmmoCapacity - 1);

    // When the shell reaches the locked impact point.
    projectiles_.update(2.0F);

    // Then it explodes over the wall and hits the target.
    EXPECT_EQ(world_.tryGet<model::components::Health>(target)->current, 85);
}
