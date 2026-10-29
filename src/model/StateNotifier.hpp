#pragma once

#include "core/EventBus.hpp"
#include "model/components/EntityStates.hpp"
#include "model/ecs/Entity.hpp"
#include "model/ecs/Registry.hpp"

namespace model {

// Stores the state as a component and publishes the matching event only when it actually changes.
// The first call also publishes, so a freshly spawned entity starts the View synchronized.
void setUnitState(core::EventBus& bus, ecs::Registry& world, ecs::Entity entity, UnitState state);
void setPlayerState(core::EventBus& bus, ecs::Registry& world, ecs::Entity entity, PlayerState state);
void setBuildingState(core::EventBus& bus, ecs::Registry& world, ecs::Entity entity, BuildingState state);

} // namespace model
