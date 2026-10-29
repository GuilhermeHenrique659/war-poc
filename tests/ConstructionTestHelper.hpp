#pragma once

#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/build/BuildingComponents.hpp"
#include "model/build/BuildingFactory.hpp"
#include "model/build/BuildingSystem.hpp"
#include "model/build/BuildingType.hpp"
#include "model/build/ConstructionComponents.hpp"
#include "model/components/Owner.hpp"
#include "model/StateNotifier.hpp"
#include "model/ecs/Entity.hpp"
#include "model/ecs/Registry.hpp"

#include <algorithm>
#include <vector>

namespace test_support {

// Finishes a construction site immediately, publishing the same notifications as the
// ConstructionSystem. Tests of other systems use this so they do not have to simulate the whole
// material delivery pipeline just to get a functional building.
inline void completeConstruction(model::ecs::Registry& world, model::ecs::Entity entity, core::EventBus& bus)
{
    auto* site = world.tryGet<model::components::ConstructionSite>(entity);
    const auto* footprint = world.tryGet<model::components::Footprint>(entity);
    const auto* owner = world.tryGet<model::components::Owner>(entity);
    if (site == nullptr || footprint == nullptr || owner == nullptr) {
        return;
    }
    const model::BuildingType type = site->type;
    const model::GridPos origin = footprint->origin;
    const int size = footprint->size;
    const model::PlayerId player = owner->player;

    world.remove<model::components::ConstructionSite>(entity);
    model::completeBuilding(world, entity, type);
    bus.publish(events::ConstructionProgressChanged{entity, 1.0F});
    bus.publish(events::BuildingPlaced{entity, player, type, origin, size});
    model::setBuildingState(bus, world, entity, model::BuildingState::Idle);
}

// Selects and places a building, then immediately finishes its construction site (if any).
// Returns the placed entity, or 0 when the placement was rejected.
inline model::ecs::Entity placeAndComplete(model::BuildingSystem& buildings, core::EventBus& bus,
    model::ecs::Registry& world, model::PlayerId player, model::BuildingType type, model::GridPos anchor)
{
    std::vector<model::ecs::Entity> existing;
    world.each<model::components::Footprint>([&](model::ecs::Entity entity, const model::components::Footprint&) {
        const auto* site = world.tryGet<model::components::ConstructionSite>(entity);
        const auto* building = world.tryGet<model::components::Building>(entity);
        if ((site != nullptr && site->type == type) || (building != nullptr && building->type == type)) {
            existing.push_back(entity);
        }
    });

    buildings.cancel(player);
    buildings.select(player, type);
    buildings.place(player, anchor);

    model::ecs::Entity placed = 0;
    world.each<model::components::Footprint>([&](model::ecs::Entity entity, const model::components::Footprint&) {
        const auto* owner = world.tryGet<model::components::Owner>(entity);
        if (owner == nullptr || owner->player != player || entity < placed) {
            return;
        }
        if (std::find(existing.begin(), existing.end(), entity) != existing.end()) {
            return;
        }
        const auto* site = world.tryGet<model::components::ConstructionSite>(entity);
        const auto* building = world.tryGet<model::components::Building>(entity);
        if ((site != nullptr && site->type == type) || (building != nullptr && building->type == type)) {
            placed = entity;
        }
    });
    if (placed != 0) {
        completeConstruction(world, placed, bus);
    }
    return placed;
}

} // namespace test_support
