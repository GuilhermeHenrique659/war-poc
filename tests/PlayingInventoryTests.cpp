#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/PlayingModel.hpp"
#include "model/economy/Item.hpp"

#include <gtest/gtest.h>

TEST(PlayingInventoryTests, StartsWithIronAndTransfersProductionFromASelectedBuilding)
{
    // Given a new match with the player's inventory panel observed.
    core::EventBus bus;
    model::PlayingModel playing(bus);
    events::InventoryPanelChanged panel;
    events::BuildingPlaced mine{};
    bus.subscribe<events::InventoryPanelChanged>([&panel](const auto& event) { panel = event; });
    bus.subscribe<events::BuildingPlaced>([&mine](const auto& event) {
        if (event.type == model::BuildingType::IronMine) {
            mine = event;
        }
    });
    bus.publish(events::StartMatchRequested{"Tester"});

    // When the Command Center and a mine on the starting iron deposit are placed and selected.
    bus.publish(events::BuildSelectRequested{1, model::BuildingType::CommandCenter});
    bus.publish(events::BuildPlaceRequested{1, {0, 0}});
    bus.publish(events::BuildSelectRequested{1, model::BuildingType::IronMine});
    bus.publish(events::BuildPlaceRequested{1, {-9, 0}});
    bus.publish(events::SelectBuildingRequested{1, mine.entity});
    playing.update(1.0F);

    // Then the player has 400 iron after paying for the mine, which stores one more iron on update.
    ASSERT_TRUE(panel.open);
    EXPECT_EQ(panel.buildingSlots.size(), 64U);
    EXPECT_EQ(panel.playerSlots.size(), 32U);
    int playerIron = 0;
    for (const auto& slot : panel.playerSlots) {
        if (slot.occupied && slot.item == model::ItemId::Iron) {
            playerIron += slot.currentQuantity;
        }
    }
    EXPECT_EQ(playerIron, 400);

    bus.publish(events::TransferInventoryItemRequested{1, mine.entity, 0, false});
    playerIron = 0.0F;
    for (const auto& slot : panel.playerSlots) {
        if (slot.occupied && slot.item == model::ItemId::Iron) {
            playerIron += slot.currentQuantity;
        }
    }
    EXPECT_EQ(playerIron, 401);
    EXPECT_FALSE(panel.buildingSlots[0].occupied);

    // When a stack is dragged from the player inventory into the selected building.
    bus.publish(events::TransferInventoryItemRequested{1, mine.entity, 1, true});

    // Then the player stack moves to the mine, respecting its stack capacity.
    playerIron = 0.0F;
    for (const auto& slot : panel.playerSlots) {
        if (slot.occupied && slot.item == model::ItemId::Iron) {
            playerIron += slot.currentQuantity;
        }
    }
    EXPECT_EQ(playerIron, 301);
    EXPECT_TRUE(panel.buildingSlots[0].occupied);
    EXPECT_EQ(panel.buildingSlots[0].currentQuantity, 100);

    // When the selected building is deselected and the player opens their inventory.
    bus.publish(events::SelectionClearRequested{1});
    bus.publish(events::TogglePlayerInventoryRequested{1});

    // Then the player inventory opens by itself and can be toggled closed.
    ASSERT_TRUE(panel.open);
    EXPECT_TRUE(panel.playerVisible);
    EXPECT_TRUE(panel.playerInventoryOpen);
    EXPECT_FALSE(panel.hasBuilding);
    bus.publish(events::TogglePlayerInventoryRequested{1});
    EXPECT_FALSE(panel.open);
}
