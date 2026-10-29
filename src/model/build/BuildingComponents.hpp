#pragma once

#include "model/build/BuildingType.hpp"
#include "model/components/CommonComponents.hpp"

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

} // namespace model::components
