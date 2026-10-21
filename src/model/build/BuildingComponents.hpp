#pragma once

#include "model/build/BuildingType.hpp"
#include "model/GridPos.hpp"
#include "model/components/Owner.hpp"

#include <string>

namespace model::components {

struct Building {
    explicit Building(BuildingType buildingType)
        : type(buildingType)
        , name(buildingSpec(buildingType).label)
    {
    }

    BuildingType type;
    std::string name;
};

struct Footprint {
    GridPos origin;
    int size;
};

} // namespace model::components
