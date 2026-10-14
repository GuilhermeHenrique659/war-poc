#pragma once

#include "model/BuildingType.hpp"
#include "model/GridPos.hpp"
#include "model/components/Owner.hpp"

namespace model::components {

struct Building {
    BuildingType type;
};

struct Footprint {
    GridPos origin;
    int size;
};

} // namespace model::components
