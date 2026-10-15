#pragma once

#include "model/GridPos.hpp"

#include <array>
#include <string_view>

namespace model {

enum class BuildingType { CommandCenter, Wall, House, Generator, Warehouse, Barracks, Tower };

// Order in which the build bar lists the buildings.
inline constexpr std::array kBuildingTypes{BuildingType::CommandCenter, BuildingType::Wall, BuildingType::House,
    BuildingType::Generator, BuildingType::Warehouse, BuildingType::Barracks, BuildingType::Tower};

struct BuildingSpec {
    std::string_view label;
    int size; // Square footprint, in tiles per side.
};

[[nodiscard]] constexpr BuildingSpec buildingSpec(BuildingType type)
{
    switch (type) {
    case BuildingType::CommandCenter:
        return BuildingSpec{"Centro de Comando", 5};
    case BuildingType::Wall:
        return BuildingSpec{"Muro", 1};
    case BuildingType::House:
        return BuildingSpec{"Casa", 2};
    case BuildingType::Generator:
        return BuildingSpec{"Gerador", 2};
    case BuildingType::Warehouse:
        return BuildingSpec{"Deposito", 2};
    case BuildingType::Barracks:
        return BuildingSpec{"Quartel", 3};
    case BuildingType::Tower:
        return BuildingSpec{"Torre", 2};
    }
    return BuildingSpec{"?", 1};
}

// The cursor tile is the center of the footprint.
[[nodiscard]] constexpr GridPos footprintOrigin(GridPos anchor, int size)
{
    return GridPos{anchor.x - size / 2, anchor.y - size / 2};
}

} // namespace model
