#include "model/player/PlayerInventory.hpp"

#include "model/economy/InventorySystem.hpp"

#include <gtest/gtest.h>

namespace {

constexpr model::PlayerId kPlayer = 7;
constexpr model::PlayerId kPlayerWithoutAvatarInventory = 9;

} // namespace

TEST(PlayerInventoryTests, ReturnsTheInventoryOfThePlayersAvatarEntity)
{
    // Given a registry with two players; only one has an inventory on its avatar.
    model::ecs::Registry world;
    const model::ecs::Entity withInventory = world.create();
    world.emplace<model::components::PlayerProfile>(withInventory, kPlayer, "Jogador");
    auto& inventory = world.emplace<model::components::Inventory>(withInventory, std::size_t{4});
    (void)model::InventorySystem::add(inventory, model::ItemId::Iron, 10);

    const model::ecs::Entity withoutInventory = world.create();
    world.emplace<model::components::PlayerProfile>(withoutInventory, kPlayerWithoutAvatarInventory, "Sem");

    // When looking the inventory up by player.
    model::components::Inventory* found = model::playerInventory(world, kPlayer);
    model::components::Inventory* missing = model::playerInventory(world, kPlayerWithoutAvatarInventory);
    model::components::Inventory* unknown = model::playerInventory(world, 123);

    // Then only the player that owns an inventory is found.
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(model::InventorySystem::amount(*found, model::ItemId::Iron), 10);
    EXPECT_EQ(missing, nullptr);
    EXPECT_EQ(unknown, nullptr);
}
