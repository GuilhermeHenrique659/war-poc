#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/SelectionSystem.hpp"
#include "model/UnitSystem.hpp"
#include "model/BuildingSystem.hpp"

#include <gtest/gtest.h>

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
