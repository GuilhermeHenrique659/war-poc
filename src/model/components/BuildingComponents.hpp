#pragma once

#include "model/BuildingType.hpp"
#include "model/GridPos.hpp"
#include "model/Player.hpp"

namespace model::components {

struct Owner {
    PlayerId player;
};

struct Building {
    BuildingType type;
};

struct Footprint {
    GridPos origin;
    int size;
};

} // namespace model::components
