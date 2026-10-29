#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/Direction.hpp"
#include "model/build/BuildingComponents.hpp"
#include "model/combat/WeaponComponents.hpp"
#include "model/player/PlayerAvatarSystem.hpp"
#include "model/components/CommonComponents.hpp"
#include "model/components/Owner.hpp"
#include "model/economy/InventoryComponents.hpp"
#include "model/economy/InventorySystem.hpp"
#include "model/economy/Item.hpp"
#include "model/player/PlayerComponents.hpp"
#include "model/units/UnitComponents.hpp"
#include "model/ecs/Registry.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <vector>

namespace {

class PlayerAvatarSystemTest : public ::testing::Test {
protected:
    PlayerAvatarSystemTest()
        : players_(bus_, world_)
    {
        bus_.subscribe<events::FacingChanged>([this](const events::FacingChanged& event) {
            facings_.push_back(event);
        });
    }

    core::EventBus bus_;
    model::ecs::Registry world_;
    model::PlayerAvatarSystem players_;
    std::vector<events::FacingChanged> facings_;
};

} // namespace

TEST_F(PlayerAvatarSystemTest, CreatesOnlyThePlayerIdentityUntilTheCommandCenterIsPlaced)
{
    // Given an empty world.
    constexpr model::PlayerId playerId = 7;

    // When the player identity is created before placing its Command Center.
    const model::ecs::Entity entity = players_.create(playerId, "Jogador");

    // Then its identity exists, but it has not spawned into the world.
    const auto* profile = world_.tryGet<model::components::PlayerProfile>(entity);
    const auto* owner = world_.tryGet<model::components::Owner>(entity);
    const auto* position = world_.tryGet<model::components::Position>(entity);
    const auto* health = world_.tryGet<model::components::Health>(entity);
    const auto* weapon = world_.tryGet<model::components::Weapon>(entity);
    ASSERT_NE(profile, nullptr);
    ASSERT_NE(owner, nullptr);
    EXPECT_EQ(profile->id, playerId);
    EXPECT_EQ(profile->name, "Jogador");
    EXPECT_EQ(owner->player, playerId);
    EXPECT_EQ(position, nullptr);
    EXPECT_EQ(health, nullptr);
    EXPECT_EQ(weapon, nullptr);
}

TEST_F(PlayerAvatarSystemTest, StartsWithWeaponsAndAmmoToSupplyItsUnits)
{
    // Given an empty world.
    constexpr model::PlayerId playerId = 7;

    // When the player identity is created.
    const model::ecs::Entity entity = players_.create(playerId, "Jogador");

    // Then the sandbox inventory has weapons and ammo to transfer to storage later.
    const auto* inventory = world_.tryGet<model::components::Inventory>(entity);
    ASSERT_NE(inventory, nullptr);
    EXPECT_EQ(model::InventorySystem::amount(*inventory, model::ItemId::Weapon), 10);
    EXPECT_EQ(model::InventorySystem::amount(*inventory, model::ItemId::Ammo), 1000);
}

TEST_F(PlayerAvatarSystemTest, SpawnsAtTheCommandCenterWhenItIsPlaced)
{
    // Given a player identity without a spawned avatar.
    constexpr model::PlayerId playerId = 7;
    const model::ecs::Entity entity = players_.create(playerId, "Jogador");
    events::PlayerSpawned spawned{};
    int spawnCount = 0;
    bus_.subscribe<events::PlayerSpawned>([&](const auto& event) {
        spawned = event;
        ++spawnCount;
    });

    // When the player places a 5x5 Command Center.
    bus_.publish(events::BuildingPlaced{99, playerId, model::BuildingType::CommandCenter, {2, 4}, 5});

    // Then the avatar spawns at the Command Center center with its combat attributes.
    const auto* position = world_.tryGet<model::components::Position>(entity);
    const auto* health = world_.tryGet<model::components::Health>(entity);
    const auto* weapon = world_.tryGet<model::components::Weapon>(entity);
    ASSERT_NE(position, nullptr);
    ASSERT_NE(health, nullptr);
    ASSERT_NE(weapon, nullptr);
    EXPECT_FLOAT_EQ(position->x, 4.5F);
    EXPECT_FLOAT_EQ(position->y, 1.5F);
    EXPECT_EQ(health->current, 10);
    EXPECT_EQ(health->max, 10);
    EXPECT_EQ(weapon->type, model::WeaponType::Pistol);
    EXPECT_EQ(weapon->damage, 1);
    EXPECT_FLOAT_EQ(weapon->range, 3.0F);
    ASSERT_EQ(spawnCount, 1);
    EXPECT_EQ(spawned.entity, entity);
    EXPECT_EQ(spawned.player, playerId);
    EXPECT_FLOAT_EQ(spawned.x, 4.5F);
    EXPECT_FLOAT_EQ(spawned.y, 1.5F);
}

TEST_F(PlayerAvatarSystemTest, IgnoresOtherBuildingTypesWhenWaitingToSpawn)
{
    // Given a player identity that has not spawned.
    constexpr model::PlayerId playerId = 7;
    const model::ecs::Entity entity = players_.create(playerId, "Jogador");

    // When the player places a non-Command-Center building.
    bus_.publish(events::BuildingPlaced{99, playerId, model::BuildingType::House, {2, 4}, 2});

    // Then the avatar remains unspawned.
    EXPECT_EQ(world_.tryGet<model::components::Position>(entity), nullptr);
    EXPECT_EQ(world_.tryGet<model::components::Health>(entity), nullptr);
}

TEST_F(PlayerAvatarSystemTest, MovesTheAvatarAtEngineerSpeedAndPublishesItsPosition)
{
    // Given a player avatar whose Command Center is centered at (4, 6).
    constexpr model::PlayerId playerId = 3;
    const model::ecs::Entity entity = players_.create(playerId, "Jogador");
    bus_.publish(events::BuildingPlaced{99, playerId, model::BuildingType::CommandCenter, {2, 4}, 5});
    events::PlayerMoved movement{};
    bus_.subscribe<events::PlayerMoved>([&](const auto& event) { movement = event; });

    // When the player requests one second of movement along the X grid axis.
    bus_.publish(events::PlayerMoveRequested{playerId, 1.0F, 0.0F, 1.0F});

    // Then it moves at Engineer speed and publishes the updated position.
    const auto* position = world_.tryGet<model::components::Position>(entity);
    ASSERT_NE(position, nullptr);
    EXPECT_FLOAT_EQ(position->x, 6.0F);
    EXPECT_FLOAT_EQ(position->y, 1.5F);
    EXPECT_EQ(movement.entity, entity);
    EXPECT_EQ(movement.player, playerId);
    EXPECT_FLOAT_EQ(movement.x, 6.0F);
    EXPECT_FLOAT_EQ(movement.y, 1.5F);
}

TEST_F(PlayerAvatarSystemTest, RespawnsAPlayerAtItsStartingPositionWithFullHealth)
{
    // Given a player avatar that has moved and lost all its health after placing a Command Center.
    constexpr model::PlayerId playerId = 2;
    const model::ecs::Entity entity = players_.create(playerId, "Jogador");
    bus_.publish(events::BuildingPlaced{99, playerId, model::BuildingType::CommandCenter, {5, -3}, 5});
    world_.tryGet<model::components::Position>(entity)->x = 12.0F;
    world_.tryGet<model::components::Position>(entity)->y = -4.0F;
    world_.tryGet<model::components::Health>(entity)->current = 0;

    // When the combat system requests a respawn.
    bus_.publish(events::PlayerRespawnRequested{entity});

    // Then the avatar returns to its initial position with full health.
    const auto* position = world_.tryGet<model::components::Position>(entity);
    const auto* health = world_.tryGet<model::components::Health>(entity);
    ASSERT_NE(position, nullptr);
    ASSERT_NE(health, nullptr);
    EXPECT_FLOAT_EQ(position->x, 7.5F);
    EXPECT_FLOAT_EQ(position->y, -1.5F);
    EXPECT_EQ(health->current, 10);
}

TEST_F(PlayerAvatarSystemTest, DiagonalMovementDoesNotExceedEngineerSpeed)
{
    // Given a player avatar spawned from its Command Center.
    constexpr model::PlayerId playerId = 9;
    const model::ecs::Entity entity = players_.create(playerId, "Jogador");
    bus_.publish(events::BuildingPlaced{99, playerId, model::BuildingType::CommandCenter, {0, 0}, 5});
    const auto* start = world_.tryGet<model::components::Position>(entity);
    ASSERT_NE(start, nullptr);
    const float startX = start->x;
    const float startY = start->y;

    // When it moves diagonally for one second.
    players_.move(playerId, 1.0F, 1.0F, 1.0F);

    // Then its total displacement equals the Engineer speed, not a faster diagonal.
    const auto* position = world_.tryGet<model::components::Position>(entity);
    ASSERT_NE(position, nullptr);
    EXPECT_FLOAT_EQ(std::hypot(position->x - startX, position->y - startY), 1.5F);
}

TEST_F(PlayerAvatarSystemTest, StopsAtBuildingCollisionAndSlidesAlongItsWall)
{
    // Given an avatar outside its Command Center and a house ahead of it.
    constexpr model::PlayerId playerId = 11;
    const model::ecs::Entity avatar = players_.create(playerId, "Jogador");
    bus_.publish(events::BuildingPlaced{99, playerId, model::BuildingType::CommandCenter, {0, 0}, 5});
    auto* position = world_.tryGet<model::components::Position>(avatar);
    ASSERT_NE(position, nullptr);
    position->x = 0.5F;
    position->y = 0.5F;
    const model::ecs::Entity house = world_.create();
    world_.emplace<model::components::Building>(house, model::BuildingType::House);
    world_.emplace<model::components::Footprint>(house, model::GridPos{2, 0}, 2);

    // When it moves towards and along the house's west wall.
    players_.move(playerId, 1.0F, 1.0F, 1.0F);

    // Then it stops at the collision boundary without passing through the house.
    EXPECT_FLOAT_EQ(position->x, 1.56F);
    EXPECT_FLOAT_EQ(position->y, 0.5F + 1.5F / std::sqrt(2.0F));
}

TEST_F(PlayerAvatarSystemTest, StopsAtWaterInsteadOfWalkingAcrossIt)
{
    // Given an avatar on land with water beginning at tile x=4.
    constexpr model::PlayerId playerId = 12;
    const model::ecs::Entity avatar = players_.create(playerId, "Jogador");
    bus_.publish(events::BuildingPlaced{99, playerId, model::BuildingType::CommandCenter, {0, 0}, 5});
    players_.setTileWalkable([](model::GridPos tile) { return tile.x < 4; });

    // When it moves east for one second.
    players_.move(playerId, 1.0F, 0.0F, 1.0F);

    // Then its collision radius stops before the water tile.
    const auto* position = world_.tryGet<model::components::Position>(avatar);
    ASSERT_NE(position, nullptr);
    EXPECT_LT(position->x + 0.3F, 4.001F);
    EXPECT_GT(position->x, 1.5F);
}

TEST_F(PlayerAvatarSystemTest, SpawnsFacingSouthAndPublishesTheInitialFacing)
{
    // Given a player identity without a spawned avatar.
    constexpr model::PlayerId playerId = 7;
    const model::ecs::Entity entity = players_.create(playerId, "Jogador");

    // When the player places a Command Center.
    bus_.publish(events::BuildingPlaced{99, playerId, model::BuildingType::CommandCenter, {2, 4}, 5});

    // Then the avatar spawns facing south and the View receives that initial state.
    const auto* facing = world_.tryGet<model::components::Facing>(entity);
    ASSERT_NE(facing, nullptr);
    EXPECT_EQ(facing->direction, model::Direction::South);
    ASSERT_EQ(facings_.size(), 1U);
    EXPECT_EQ(facings_.front().entity, entity);
    EXPECT_EQ(facings_.front().direction, model::Direction::South);
}

TEST_F(PlayerAvatarSystemTest, TurnsToFaceTheDirectionThePlayerMoves)
{
    // Given a spawned avatar standing at its Command Center.
    constexpr model::PlayerId playerId = 3;
    const model::ecs::Entity entity = players_.create(playerId, "Jogador");
    bus_.publish(events::BuildingPlaced{99, playerId, model::BuildingType::CommandCenter, {2, 4}, 5});
    facings_.clear();

    // When the player moves east.
    bus_.publish(events::PlayerMoveRequested{playerId, 1.0F, 0.0F, 1.0F});

    // Then the avatar faces east and publishes the new facing.
    const auto* facing = world_.tryGet<model::components::Facing>(entity);
    ASSERT_NE(facing, nullptr);
    EXPECT_EQ(facing->direction, model::Direction::East);
    ASSERT_EQ(facings_.size(), 1U);
    EXPECT_EQ(facings_.front().entity, entity);
    EXPECT_EQ(facings_.front().direction, model::Direction::East);
}

TEST_F(PlayerAvatarSystemTest, KeepsTheFacingWhenThePlayerStandsStill)
{
    // Given a spawned avatar.
    constexpr model::PlayerId playerId = 4;
    const model::ecs::Entity entity = players_.create(playerId, "Jogador");
    bus_.publish(events::BuildingPlaced{99, playerId, model::BuildingType::CommandCenter, {2, 4}, 5});
    facings_.clear();

    // When the player sends a movement request with no direction.
    bus_.publish(events::PlayerMoveRequested{playerId, 0.0F, 0.0F, 1.0F});

    // Then it keeps facing south and no notification is published.
    const auto* facing = world_.tryGet<model::components::Facing>(entity);
    ASSERT_NE(facing, nullptr);
    EXPECT_EQ(facing->direction, model::Direction::South);
    EXPECT_TRUE(facings_.empty());
}
