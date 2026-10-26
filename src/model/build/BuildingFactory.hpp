#pragma once

#include "model/build/BuildingType.hpp"
#include "model/ecs/Entity.hpp"
#include "model/ecs/Registry.hpp"

namespace model {

// Turns a construction shell into a functional building: adds the Building component and the
// production/weapon behaviour fixed by its type. Safe to call once per entity.
void completeBuilding(ecs::Registry& world, ecs::Entity entity, BuildingType type);

} // namespace model
