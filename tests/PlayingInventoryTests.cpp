#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/PlayingModel.hpp"
#include "model/economy/Item.hpp"

#include <gtest/gtest.h>

namespace {

int amountOf(const events::InventoryPaneState& pane, model::ItemId item)
{
    int amount = 0;
    for (const auto& slot : pane.slots) {
        if (slot.occupied && slot.item == item) {
            amount += slot.currentQuantity;
        }
    }
    return amount;
}

} // namespace

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

    // Then the player and the mine are two panes of the same generic panel.
    const auto playerAmount = [&panel](model::ItemId item) {
        const auto* pane = events::findPane(panel, events::InventoryPaneRole::Player);
        return pane == nullptr ? -1 : amountOf(*pane, item);
    };
    const auto buildingPane = [&panel]() {
        return events::findPane(panel, events::InventoryPaneRole::TransferTarget);
    };
    ASSERT_TRUE(panel.open);
    ASSERT_NE(events::findPane(panel, events::InventoryPaneRole::Player), nullptr);
    ASSERT_NE(buildingPane(), nullptr);
    EXPECT_EQ(buildingPane()->entity, mine.entity);
    EXPECT_EQ(buildingPane()->layout, events::InventoryPaneLayout::Slots);
    EXPECT_EQ(buildingPane()->slots.size(), 64U);
    EXPECT_EQ(events::findPane(panel, events::InventoryPaneRole::Player)->slots.size(), 32U);
    EXPECT_EQ(playerAmount(model::ItemId::Iron), 400);

    bus.publish(events::TransferInventoryItemRequested{1, mine.entity, 0, false});
    EXPECT_EQ(playerAmount(model::ItemId::Iron), 401);
    EXPECT_FALSE(buildingPane()->slots[0].occupied);

    // When a stack is dragged from the player inventory into the selected building.
    bus.publish(events::TransferInventoryItemRequested{1, mine.entity, 1, true});

    // Then the player stack moves to the mine, respecting its stack capacity.
    EXPECT_EQ(playerAmount(model::ItemId::Iron), 301);
    EXPECT_TRUE(buildingPane()->slots[0].occupied);
    EXPECT_EQ(buildingPane()->slots[0].currentQuantity, 100);

    // When the selected building is deselected and the player opens their inventory.
    bus.publish(events::SelectionClearRequested{1});
    bus.publish(events::TogglePlayerInventoryRequested{1});

    // Then only the player pane remains open.
    ASSERT_TRUE(panel.open);
    EXPECT_TRUE(panel.playerInventoryOpen);
    EXPECT_EQ(panel.panes.size(), 1U);
    EXPECT_EQ(events::findPane(panel, events::InventoryPaneRole::TransferTarget), nullptr);
    bus.publish(events::TogglePlayerInventoryRequested{1});
    EXPECT_FALSE(panel.open);
}
