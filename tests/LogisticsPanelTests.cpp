#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/PlayingModel.hpp"

#include <gtest/gtest.h>

#include <algorithm>

TEST(LogisticsPanelTests, PublishesBuildingAndRouteTypesInsteadOfNames)
{
    // Given a match with a warehouse and an iron mine that feeds it.
    core::EventBus bus;
    model::PlayingModel playing(bus);
    events::LogisticsPanelChanged panel{};
    model::ecs::Entity warehouse = 0;
    model::ecs::Entity mine = 0;
    bus.subscribe<events::LogisticsPanelChanged>([&](const events::LogisticsPanelChanged& event) {
        if (event.player == 1) {
            panel = event;
        }
    });
    bus.subscribe<events::BuildingPlaced>([&](const events::BuildingPlaced& event) {
        if (event.type == model::BuildingType::Warehouse) {
            warehouse = event.entity;
        }
        if (event.type == model::BuildingType::IronMine) {
            mine = event.entity;
        }
    });
    bus.publish(events::StartMatchRequested{"Tester"});
    bus.publish(events::DebugToggleRequested{});
    bus.publish(events::BuildSelectRequested{1, model::BuildingType::CommandCenter});
    bus.publish(events::BuildPlaceRequested{1, {0, 0}});
    bus.publish(events::BuildSelectRequested{1, model::BuildingType::Warehouse});
    bus.publish(events::BuildPlaceRequested{1, {6, 0}});
    bus.publish(events::BuildSelectRequested{1, model::BuildingType::IronMine});
    bus.publish(events::BuildPlaceRequested{1, {-9, 0}});
    playing.update(9.0F);
    ASSERT_NE(warehouse, 0U);
    ASSERT_NE(mine, 0U);

    // When the producing building is selected.
    bus.publish(events::SelectBuildingRequested{1, mine});

    // Then the panel carries the building type and the free flag, not text.
    ASSERT_TRUE(panel.open);
    EXPECT_EQ(panel.building, mine);
    EXPECT_EQ(panel.buildingType, model::BuildingType::IronMine);
    EXPECT_TRUE(panel.free);
    EXPECT_FALSE(panel.warehouse);

    // And each route names its destination by type.
    const auto route = std::find_if(panel.routes.begin(), panel.routes.end(),
        [warehouse](const events::LogisticsRouteEntry& entry) { return entry.destination == warehouse; });
    ASSERT_NE(route, panel.routes.end());
    EXPECT_EQ(route->destinationType, model::BuildingType::Warehouse);
    EXPECT_EQ(route->item, model::ItemId::Iron);
}
