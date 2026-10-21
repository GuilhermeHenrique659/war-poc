#include "model/economy/InventoryComponents.hpp"
#include "model/economy/InventorySystem.hpp"
#include <gtest/gtest.h>

#include <type_traits>

TEST(InventorySystemTests, AddsItemsAcrossStacksUpToSlotCapacity)
{
    // Given an inventory with two slots and a stack limit of 100.
    model::components::Inventory inventory{2};

    // When 250 iron are added.
    const int added = model::InventorySystem::add(inventory, model::ItemId::Iron, 250);

    // Then two full stacks are accepted and the remaining 50 are rejected.
    EXPECT_EQ(added, 200);
    ASSERT_TRUE(inventory.slots[0].has_value());
    ASSERT_TRUE(inventory.slots[1].has_value());
    EXPECT_EQ(inventory.slots[0]->currentQuantity, 100);
    EXPECT_EQ(inventory.slots[1]->currentQuantity, 100);
}

TEST(InventorySystemTests, RejectsFractionalItemQuantities)
{
    // Given inventory quantities are integral in the model type.
    static_assert(std::is_same_v<decltype(model::components::InventoryItem::currentQuantity), int>);
    static_assert(std::is_same_v<decltype(model::components::InventoryItem::maxQuantity), int>);

    // When the inventory item type is inspected.
    using Quantity = decltype(model::components::InventoryItem::currentQuantity);

    // Then inventory quantities cannot represent fractional items.
    EXPECT_TRUE((std::is_same_v<Quantity, int>));
}

TEST(InventorySystemTests, TransfersOnlyWhatFitsInDestination)
{
    // Given a source with 75 iron and a one-slot destination containing 50 coal.
    model::components::Inventory source{2};
    model::components::Inventory destination{1};
    model::InventorySystem::add(source, model::ItemId::Iron, 75);
    model::InventorySystem::add(destination, model::ItemId::Coal, 50);

    // When the source iron is transferred.
    const int transferred = model::InventorySystem::transfer(source, destination, model::ItemId::Iron, 75);

    // Then the transfer is rejected because the destination has no empty slot.
    EXPECT_EQ(transferred, 0);
    EXPECT_EQ(model::InventorySystem::amount(source, model::ItemId::Iron), 75);
    EXPECT_EQ(model::InventorySystem::amount(destination, model::ItemId::Iron), 0);
}

TEST(InventorySystemTests, TransfersAvailableAmountIntoExistingStack)
{
    // Given an iron stack in each inventory with room in the destination.
    model::components::Inventory source{1};
    model::components::Inventory destination{1};
    model::InventorySystem::add(source, model::ItemId::Iron, 80);
    model::InventorySystem::add(destination, model::ItemId::Iron, 70);

    // When all source iron is transferred.
    const int transferred = model::InventorySystem::transfer(source, destination, model::ItemId::Iron, 80);

    // Then only the 30 units that fit are moved.
    EXPECT_EQ(transferred, 30);
    EXPECT_EQ(model::InventorySystem::amount(source, model::ItemId::Iron), 50);
    EXPECT_EQ(model::InventorySystem::amount(destination, model::ItemId::Iron), 100);
}
