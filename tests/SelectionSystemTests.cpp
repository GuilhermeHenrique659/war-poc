#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/build/BuildingComponents.hpp"
#include "model/build/BuildingSystem.hpp"
#include "model/build/ConstructionComponents.hpp"
#include "model/components/Owner.hpp"
#include "model/selection/SelectionSystem.hpp"
#include "model/units/UnitSystem.hpp"

#include <gtest/gtest.h>

#include <algorithm>

TEST(SelectionSystemTest, RemovesDestroyedUnitFromPublishedSelection)
{
    // Given a selected unit and a selection observer.
    core::EventBus bus;
    model::ecs::Registry world;
    model::BuildingSystem buildings(bus, world);
    model::UnitSystem units(bus, world, buildings);
    model::SelectionSystem selection(bus, world);
    const auto unit = units.spawn(1, model::recruitStats(model::UnitType::Soldier), {0, 0});
    selection.selectUnits(1, {unit}, false);
    std::vector<events::SelectionChanged> changes;
    bus.subscribe<events::SelectionChanged>([&](const auto& event) { changes.push_back(event); });

    // When the model reports that the unit was removed.
    units.remove(unit);

    // Then the selection notification no longer contains the dead entity.
    ASSERT_FALSE(changes.empty());
    EXPECT_TRUE(changes.back().units.empty());
}

TEST(SelectionSystemTest, RecruitPanelPublishesSemanticCostsAndDroneCapacity)
{
    // Given a player-owned Command Center and an observer of the recruit panel.
    core::EventBus bus;
    model::ecs::Registry world;
    model::SelectionSystem selection(bus, world);
    events::RecruitPanelChanged panel{};
    bus.subscribe<events::RecruitPanelChanged>([&](const auto& event) { panel = event; });
    const auto commandCenter = world.create();
    world.emplace<model::components::Building>(commandCenter, model::BuildingType::CommandCenter);
    world.emplace<model::components::Owner>(commandCenter, model::PlayerId{1});

    // When the Command Center is selected.
    selection.selectBuilding(1, commandCenter);

    // Then the panel is open and the construction drone reports capacity and materials as data.
    ASSERT_TRUE(panel.open);
    const auto drone = std::find_if(panel.entries.begin(), panel.entries.end(),
        [](const events::RecruitEntry& entry) {
            return entry.kind == events::RecruitEntryKind::ConstructionDrone;
        });
    ASSERT_NE(drone, panel.entries.end());
    EXPECT_EQ(drone->capacity, model::kConstructionDroneCapacity);
    EXPECT_FALSE(drone->free);
    ASSERT_EQ(drone->costs.size(), 2U);
    EXPECT_EQ(drone->costs[0].item, model::ItemId::Iron);
    EXPECT_EQ(drone->costs[0].quantity, model::kConstructionDroneIronCost);
    EXPECT_EQ(drone->costs[1].item, model::ItemId::Electronics);
    EXPECT_EQ(drone->costs[1].quantity, model::kConstructionDroneElectronicsCost);

    // When debug mode is enabled, the same entry is flagged as free instead of carrying text.
    bus.publish(events::DebugToggleRequested{});
    const auto freeDrone = std::find_if(panel.entries.begin(), panel.entries.end(),
        [](const events::RecruitEntry& entry) {
            return entry.kind == events::RecruitEntryKind::ConstructionDrone;
        });
    ASSERT_NE(freeDrone, panel.entries.end());
    EXPECT_TRUE(freeDrone->free);
}
