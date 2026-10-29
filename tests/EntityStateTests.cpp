#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/build/BuildingComponents.hpp"
#include "model/build/BuildingSystem.hpp"
#include "model/build/ConstructionSystem.hpp"
#include "model/combat/DamageSystem.hpp"
#include "model/combat/ProjectileSystem.hpp"
#include "model/components/EntityStates.hpp"
#include "model/economy/InventoryComponents.hpp"
#include "model/economy/InventorySystem.hpp"
#include "model/economy/Item.hpp"
#include "model/economy/ProductionSystem.hpp"
#include "model/player/PlayerAvatarSystem.hpp"
#include "model/player/PlayerComponents.hpp"
#include "model/units/CombatSystem.hpp"
#include "model/units/UnitSystem.hpp"
#include "ConstructionTestHelper.hpp"

#include <gtest/gtest.h>

#include <vector>

namespace {

class UnitStateTest : public ::testing::Test {
protected:
    UnitStateTest()
        : buildings_(bus_, world_)
        , units_(bus_, world_, buildings_)
        , damage_(bus_, world_, units_, buildings_)
        , projectiles_(bus_, world_, damage_)
        , combat_(bus_, world_, units_, buildings_, damage_, projectiles_)
    {
        bus_.subscribe<events::UnitStateChanged>([this](const events::UnitStateChanged& event) {
            states_.push_back(event);
        });
    }

    [[nodiscard]] model::UnitState stateOf(model::ecs::Entity entity)
    {
        const auto* state = world_.tryGet<model::UnitState>(entity);
        return state == nullptr ? model::UnitState::Idle : *state;
    }

    core::EventBus bus_;
    model::ecs::Registry world_;
    model::BuildingSystem buildings_;
    model::UnitSystem units_;
    model::DamageSystem damage_;
    model::ProjectileSystem projectiles_;
    model::CombatSystem combat_;
    std::vector<events::UnitStateChanged> states_;
    static constexpr model::PlayerId kPlayer = 1;
};

} // namespace

TEST_F(UnitStateTest, SpawnsAUnitIdleAndPublishesTheInitialState)
{
    // Given a Soldier that is about to be recruited.
    const model::UnitStats soldier = model::recruitStats(model::UnitType::Soldier);

    // When it spawns into the world.
    const model::ecs::Entity unit = units_.spawn(kPlayer, soldier, {0, 0});

    // Then it starts idle and the View is told about that initial state.
    EXPECT_EQ(stateOf(unit), model::UnitState::Idle);
    ASSERT_EQ(states_.size(), 1U);
    EXPECT_EQ(states_.front().entity, unit);
    EXPECT_EQ(states_.front().state, model::UnitState::Idle);
}

TEST_F(UnitStateTest, UnitBecomesMovingWhenItReceivesARouteAndIdleWhenItArrives)
{
    // Given a Soldier standing at the origin.
    const model::UnitStats soldier = model::recruitStats(model::UnitType::Soldier);
    const model::ecs::Entity unit = units_.spawn(kPlayer, soldier, {0, 0});
    states_.clear();

    // When it receives a move order.
    units_.orderMove(kPlayer, {unit}, {2, 0});

    // Then it is moving and the change was published.
    EXPECT_EQ(stateOf(unit), model::UnitState::Moving);
    ASSERT_EQ(states_.size(), 1U);
    EXPECT_EQ(states_.front().state, model::UnitState::Moving);

    // When it finishes walking the route.
    ASSERT_TRUE(units_.update(1.0F));

    // Then it is idle again and the change was published.
    EXPECT_EQ(stateOf(unit), model::UnitState::Idle);
    ASSERT_EQ(states_.size(), 2U);
    EXPECT_EQ(states_.back().state, model::UnitState::Idle);
}

TEST_F(UnitStateTest, UnitBecomesAttackingWhileItEngagesATargetInRange)
{
    // Given a Soldier and an enemy Soldier one tile apart.
    const model::UnitStats soldier = model::recruitStats(model::UnitType::Soldier);
    const model::ecs::Entity attacker = units_.spawn(kPlayer, soldier, {0, 0});
    const model::ecs::Entity target = units_.spawn(2, soldier, {1, 0});
    states_.clear();

    // When the attacker receives an order against a target already inside its range.
    combat_.orderAttack(kPlayer, {attacker}, target);
    combat_.update(0.0F);

    // Then it is attacking and the change was published.
    EXPECT_EQ(stateOf(attacker), model::UnitState::Attacking);
    ASSERT_EQ(states_.size(), 1U);
    EXPECT_EQ(states_.front().entity, attacker);
    EXPECT_EQ(states_.front().state, model::UnitState::Attacking);
}

TEST_F(UnitStateTest, UnitLeavesAttackingWhenTheAttackOrderIsReplacedByAMove)
{
    // Given a Soldier attacking an enemy in range.
    const model::UnitStats soldier = model::recruitStats(model::UnitType::Soldier);
    const model::ecs::Entity attacker = units_.spawn(kPlayer, soldier, {0, 0});
    const model::ecs::Entity target = units_.spawn(2, soldier, {1, 0});
    combat_.orderAttack(kPlayer, {attacker}, target);
    combat_.update(0.0F);
    ASSERT_EQ(stateOf(attacker), model::UnitState::Attacking);
    states_.clear();

    // When its owner orders it to walk somewhere else instead.
    units_.orderMove(kPlayer, {attacker}, {2, 0});

    // Then it is moving, not attacking anymore.
    EXPECT_EQ(stateOf(attacker), model::UnitState::Moving);
    ASSERT_EQ(states_.size(), 1U);
    EXPECT_EQ(states_.front().state, model::UnitState::Moving);
}

namespace {

class PlayerStateTest : public ::testing::Test {
protected:
    PlayerStateTest()
        : players_(bus_, world_)
    {
        bus_.subscribe<events::PlayerStateChanged>([this](const events::PlayerStateChanged& event) {
            states_.push_back(event);
        });
    }

    [[nodiscard]] model::PlayerState stateOf(model::ecs::Entity entity)
    {
        const auto* state = world_.tryGet<model::PlayerState>(entity);
        return state == nullptr ? model::PlayerState::Idle : *state;
    }

    core::EventBus bus_;
    model::ecs::Registry world_;
    model::PlayerAvatarSystem players_;
    std::vector<events::PlayerStateChanged> states_;
};

} // namespace

TEST_F(PlayerStateTest, SpawnsThePlayerIdleAndPublishesTheInitialState)
{
    // Given a player identity without a spawned avatar.
    constexpr model::PlayerId playerId = 7;
    const model::ecs::Entity entity = players_.create(playerId, "Jogador");

    // When the player places a Command Center and the avatar spawns.
    bus_.publish(events::BuildingPlaced{99, playerId, model::BuildingType::CommandCenter, {2, 4}, 5});

    // Then the avatar starts idle and the View is told about that initial state.
    EXPECT_EQ(stateOf(entity), model::PlayerState::Idle);
    ASSERT_EQ(states_.size(), 1U);
    EXPECT_EQ(states_.front().entity, entity);
    EXPECT_EQ(states_.front().state, model::PlayerState::Idle);
}

TEST_F(PlayerStateTest, PlayerBecomesMovingWhileItMovesAndIdleWhenItStops)
{
    // Given a spawned avatar.
    constexpr model::PlayerId playerId = 3;
    const model::ecs::Entity entity = players_.create(playerId, "Jogador");
    bus_.publish(events::BuildingPlaced{99, playerId, model::BuildingType::CommandCenter, {2, 4}, 5});
    states_.clear();

    // When the player moves.
    bus_.publish(events::PlayerMoveRequested{playerId, 1.0F, 0.0F, 1.0F});

    // Then it is moving and the change was published.
    EXPECT_EQ(stateOf(entity), model::PlayerState::Moving);
    ASSERT_EQ(states_.size(), 1U);
    EXPECT_EQ(states_.front().state, model::PlayerState::Moving);

    // When a tick passes without new movement.
    bus_.publish(events::PlayerMoveRequested{playerId, 0.0F, 0.0F, 1.0F});

    // Then it is idle again and the change was published.
    EXPECT_EQ(stateOf(entity), model::PlayerState::Idle);
    ASSERT_EQ(states_.size(), 2U);
    EXPECT_EQ(states_.back().state, model::PlayerState::Idle);
}

namespace {

class BuildingStateTest : public ::testing::Test {
protected:
    BuildingStateTest()
        : buildings_(bus_, world_)
        , construction_(bus_, world_)
        , production_(bus_)
    {
        bus_.subscribe<events::BuildingStateChanged>([this](const events::BuildingStateChanged& event) {
            states_.push_back(event);
        });

        const auto playerEntity = world_.create();
        world_.emplace<model::components::PlayerProfile>(playerEntity, kPlayer, "Tester");
        world_.emplace<model::components::Owner>(playerEntity, kPlayer);
        world_.emplace<model::components::Inventory>(playerEntity, std::size_t{32});
    }

    void placeCommandCenter()
    {
        buildings_.cancel(kPlayer);
        buildings_.select(kPlayer, model::BuildingType::CommandCenter);
        buildings_.place(kPlayer, {0, 0});
    }

    [[nodiscard]] model::ecs::Entity startSite(model::BuildingType type, model::GridPos anchor)
    {
        const std::size_t before = states_.size();
        buildings_.cancel(kPlayer);
        buildings_.select(kPlayer, type);
        buildings_.place(kPlayer, anchor);
        EXPECT_EQ(states_.size(), before + 1);
        return states_.size() > before ? states_.back().entity : 0;
    }

    void deliverAll(model::ecs::Entity entity, model::BuildingType type)
    {
        auto* inventory = world_.tryGet<model::components::Inventory>(entity);
        ASSERT_NE(inventory, nullptr);
        for (const model::BuildingCost& cost : model::buildingCosts(type)) {
            if (cost.quantity <= 0) {
                continue;
            }
            EXPECT_EQ(model::InventorySystem::add(*inventory, cost.item, cost.quantity), cost.quantity);
        }
    }

    [[nodiscard]] model::BuildingState stateOf(model::ecs::Entity entity)
    {
        const auto* state = world_.tryGet<model::BuildingState>(entity);
        return state == nullptr ? model::BuildingState::Idle : *state;
    }

    core::EventBus bus_;
    model::ecs::Registry world_;
    model::BuildingSystem buildings_;
    model::ConstructionSystem construction_;
    model::ProductionSystem production_;
    std::vector<events::BuildingStateChanged> states_;
    static constexpr model::PlayerId kPlayer = 1;
};

} // namespace

TEST_F(BuildingStateTest, InstantlyPlacedBuildingStartsIdle)
{
    // Given a player with no buildings.
    // When a Command Center, which is placed ready, is built.
    placeCommandCenter();

    // Then it starts idle and the View is told about that initial state.
    ASSERT_EQ(states_.size(), 1U);
    EXPECT_EQ(states_.front().state, model::BuildingState::Idle);
    EXPECT_EQ(stateOf(states_.front().entity), model::BuildingState::Idle);
}

TEST_F(BuildingStateTest, SiteStartsUnderConstructionAndBecomesIdleWhenFinished)
{
    // Given a Command Center.
    placeCommandCenter();

    // When a wall starts its construction site.
    const model::ecs::Entity wall = startSite(model::BuildingType::Wall, {10, 10});

    // Then it is under construction and the View was told about it.
    EXPECT_EQ(stateOf(wall), model::BuildingState::UnderConstruction);
    EXPECT_EQ(states_.back().state, model::BuildingState::UnderConstruction);

    // When its materials are delivered and the nominal duration elapses.
    deliverAll(wall, model::BuildingType::Wall);
    construction_.update(3.0F);

    // Then the finished building is idle and the change was published.
    EXPECT_EQ(stateOf(wall), model::BuildingState::Idle);
    EXPECT_EQ(states_.back().state, model::BuildingState::Idle);
}

TEST_F(BuildingStateTest, FactoryBecomesProducingOnlyWhileItConvertsInputs)
{
    // Given a finished steelworks with no inputs in stock.
    placeCommandCenter();
    const model::ecs::Entity steelworks = test_support::placeAndComplete(
        buildings_, bus_, world_, kPlayer, model::BuildingType::Steelworks, {10, 10});
    states_.clear();

    // When time passes without anything to convert.
    production_.update(world_, 1.0F);

    // Then it stays idle and nothing is published.
    EXPECT_EQ(stateOf(steelworks), model::BuildingState::Idle);
    EXPECT_TRUE(states_.empty());

    // When iron and coal are delivered.
    auto* inventory = world_.tryGet<model::components::Inventory>(steelworks);
    ASSERT_NE(inventory, nullptr);
    EXPECT_EQ(model::InventorySystem::add(*inventory, model::ItemId::Iron, 10), 10);
    EXPECT_EQ(model::InventorySystem::add(*inventory, model::ItemId::Coal, 10), 10);
    production_.update(world_, 1.0F);

    // Then it is producing and the change was published.
    EXPECT_EQ(stateOf(steelworks), model::BuildingState::Producing);
    ASSERT_EQ(states_.size(), 1U);
    EXPECT_EQ(states_.front().entity, steelworks);
    EXPECT_EQ(states_.front().state, model::BuildingState::Producing);

    // When the inputs run out.
    (void)model::InventorySystem::remove(*inventory, model::ItemId::Iron, 10);
    (void)model::InventorySystem::remove(*inventory, model::ItemId::Coal, 10);
    production_.update(world_, 1.0F);

    // Then it is idle again and the change was published.
    EXPECT_EQ(stateOf(steelworks), model::BuildingState::Idle);
    ASSERT_EQ(states_.size(), 2U);
    EXPECT_EQ(states_.back().state, model::BuildingState::Idle);
}
