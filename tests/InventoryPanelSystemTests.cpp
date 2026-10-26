#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/build/BuildingComponents.hpp"
#include "model/build/BuildingSystem.hpp"
#include "model/components/Owner.hpp"
#include "model/economy/InventoryComponents.hpp"
#include "model/economy/InventoryPanelSystem.hpp"
#include "model/economy/InventorySystem.hpp"
#include "model/economy/Item.hpp"
#include "model/ecs/Registry.hpp"
#include "model/player/PlayerAvatarSystem.hpp"
#include "model/selection/SelectionSystem.hpp"
#include "model/units/UnitSystem.hpp"
#include "model/units/UnitType.hpp"

#include <gtest/gtest.h>

TEST(InventoryPanelSystemTests, PublishesThePlayersInventoryAndTheSelectedUnitsWithTheSamePaneType)
{
    // Given a player with a selected, armed soldier.
    core::EventBus bus;
    model::ecs::Registry world;
    model::BuildingSystem buildings(bus, world);
    model::UnitSystem units(bus, world, buildings);
    model::SelectionSystem selection(bus, world);
    model::PlayerAvatarSystem players(bus, world);
    model::InventoryPanelSystem panels(bus, world, selection, players);
    events::InventoryPanelChanged published;
    bus.subscribe<events::InventoryPanelChanged>([&published](const auto& e) { published = e; });
    (void)players.create(1, "Tester");
    const auto soldier = units.spawn(1, model::recruitStats(model::UnitType::Soldier), {0, 0});
    auto* inventory = world.tryGet<model::components::Inventory>(soldier);
    ASSERT_NE(inventory, nullptr);
    (void)model::InventorySystem::add(*inventory, model::ItemId::Weapon, 1);
    (void)model::InventorySystem::add(*inventory, model::ItemId::Ammo, 42);

    // When the player selects the soldier.
    selection.selectUnits(1, {soldier}, false);

    // Then the panel has a player pane and a units pane built from the same structure.
    ASSERT_TRUE(published.open);
    const auto* playerPane = events::findPane(published, events::InventoryPaneRole::Player);
    const auto* unitsPane = events::findPane(published, events::InventoryPaneRole::Units);
    ASSERT_NE(playerPane, nullptr);
    ASSERT_NE(unitsPane, nullptr);
    EXPECT_FALSE(playerPane->buildingType.has_value());
    EXPECT_EQ(playerPane->layout, events::InventoryPaneLayout::Slots);
    EXPECT_EQ(unitsPane->layout, events::InventoryPaneLayout::Rows);
    ASSERT_EQ(unitsPane->units.size(), 1U);
    EXPECT_EQ(unitsPane->units[0].type, model::UnitType::Soldier);
    EXPECT_FALSE(unitsPane->buildingType.has_value());
    int ammo = 0;
    for (const auto& slot : unitsPane->units[0].slots) {
        if (slot.occupied && slot.item == model::ItemId::Ammo) {
            ammo += slot.currentQuantity;
        }
    }
    EXPECT_EQ(ammo, 42);

    // When the player closes the panel, it closes even though the unit remains selected.
    bus.publish(events::CloseInventoryRequested{1});
    EXPECT_FALSE(published.open);
}

TEST(InventoryPanelSystemTests, PublishesASelectedBuildingAsTheTransferablePane)
{
    // Given a player and an owned building with an inventory.
    core::EventBus bus;
    model::ecs::Registry world;
    model::SelectionSystem selection(bus, world);
    model::PlayerAvatarSystem players(bus, world);
    model::InventoryPanelSystem panels(bus, world, selection, players);
    (void)panels;
    events::InventoryPanelChanged published;
    bus.subscribe<events::InventoryPanelChanged>([&published](const auto& e) { published = e; });
    (void)players.create(1, "Tester");
    const auto building = world.create();
    world.emplace<model::components::Building>(building, model::BuildingType::Warehouse);
    world.emplace<model::components::Owner>(building, model::PlayerId{1});
    world.emplace<model::components::Inventory>(building, std::size_t{64});

    // When the player selects the building.
    selection.selectBuilding(1, building);

    // Then its pane is the generic transfer target.
    ASSERT_TRUE(published.open);
    const auto* buildingPane = events::findPane(published, events::InventoryPaneRole::TransferTarget);
    ASSERT_NE(buildingPane, nullptr);
    EXPECT_EQ(buildingPane->entity, building);
    EXPECT_EQ(buildingPane->layout, events::InventoryPaneLayout::Slots);
    EXPECT_EQ(buildingPane->buildingType, model::BuildingType::Warehouse);
    EXPECT_EQ(buildingPane->slots.size(), 64U);
    EXPECT_TRUE(buildingPane->showTransferHint);
}
