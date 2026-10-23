#pragma once

#include "core/Events.hpp"
#include "model/ecs/Registry.hpp"

#include <vector>

namespace model {

// Reads the supplies carried by the selected units into data the inventory panel can draw.
// Entities that are no longer live units are skipped.
[[nodiscard]] std::vector<events::UnitInventoryState> selectedUnitInventories(
    ecs::Registry& world, const std::vector<ecs::Entity>& units);

} // namespace model
