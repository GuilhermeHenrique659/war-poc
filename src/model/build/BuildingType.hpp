#pragma once

#include "model/GridPos.hpp"
#include "model/economy/Item.hpp"

#include <array>
#include <string_view>

namespace model {

enum class BuildingType {
    CommandCenter,
    Wall,
    House,
    Generator,
    Warehouse,
    Barracks,
    Cruzador,
    Tower,
    IronMine,
    CoalMine,
    Steelworks,
    CopperMine,
    ElectronicsFactory,
    ToolFactory,
    WeaponFactory,
    AmmoFactory,
    TrainingDummy
};

// Order in which the build bar lists the buildings.
inline constexpr std::array kBuildingTypes{BuildingType::CommandCenter, BuildingType::Wall, BuildingType::House,
    BuildingType::Generator, BuildingType::Warehouse, BuildingType::Barracks, BuildingType::Cruzador,
    BuildingType::Tower, BuildingType::IronMine, BuildingType::CoalMine, BuildingType::Steelworks,
    BuildingType::CopperMine, BuildingType::ElectronicsFactory, BuildingType::ToolFactory,
    BuildingType::WeaponFactory, BuildingType::AmmoFactory, BuildingType::TrainingDummy};

struct BuildingSpec {
    std::string_view label;
    int size; // Square footprint, in tiles per side.
};

struct BuildingCost {
    ItemId item = ItemId::Count;
    int quantity = 0;
};

using BuildingCosts = std::array<BuildingCost, 6>;

inline constexpr BuildingCosts kNoBuildingCost{};
inline constexpr BuildingCosts kCommonBuildingCost{{
    {ItemId::Iron, 100},
    {ItemId::Coal, 100},
    {ItemId::Steel, 100},
    {ItemId::Copper, 100},
    {ItemId::Electronics, 100},
    {ItemId::Tools, 100},
}};
inline constexpr BuildingCosts kMineBuildingCost{{{ItemId::Iron, 100}}};
inline constexpr BuildingCosts kWarehouseBuildingCost{{{ItemId::Iron, 100}}};
inline constexpr BuildingCosts kSteelworksBuildingCost{{{ItemId::Iron, 300}}};
inline constexpr BuildingCosts kElectronicsFactoryBuildingCost{
    {{ItemId::Steel, 100}, {ItemId::Iron, 200}}};
inline constexpr BuildingCosts kToolFactoryBuildingCost{
    {{ItemId::Steel, 200}, {ItemId::Iron, 200}, {ItemId::Electronics, 100}}};
inline constexpr BuildingCosts kWeaponFactoryBuildingCost{
    {{ItemId::Steel, 200}, {ItemId::Iron, 200}, {ItemId::Electronics, 100}}};
inline constexpr BuildingCosts kAmmoFactoryBuildingCost{
    {{ItemId::Steel, 200}, {ItemId::Iron, 200}, {ItemId::Copper, 100}}};

[[nodiscard]] constexpr const BuildingCosts& buildingCosts(BuildingType type)
{
    switch (type) {
    case BuildingType::CommandCenter:
    case BuildingType::TrainingDummy:
        return kNoBuildingCost;
    case BuildingType::Warehouse:
        return kWarehouseBuildingCost;
    case BuildingType::Wall:
    case BuildingType::House:
    case BuildingType::Generator:
    case BuildingType::Barracks:
    case BuildingType::Cruzador:
    case BuildingType::Tower:
        return kCommonBuildingCost;
    case BuildingType::IronMine:
    case BuildingType::CoalMine:
    case BuildingType::CopperMine:
        return kMineBuildingCost;
    case BuildingType::Steelworks:
        return kSteelworksBuildingCost;
    case BuildingType::ElectronicsFactory:
        return kElectronicsFactoryBuildingCost;
    case BuildingType::ToolFactory:
        return kToolFactoryBuildingCost;
    case BuildingType::WeaponFactory:
        return kWeaponFactoryBuildingCost;
    case BuildingType::AmmoFactory:
        return kAmmoFactoryBuildingCost;
    }
    return kNoBuildingCost;
}

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
        return BuildingSpec{"Armazem", 2};
    case BuildingType::Barracks:
        return BuildingSpec{"Quartel", 3};
    case BuildingType::Cruzador:
        return BuildingSpec{"Cruzador", 3};
    case BuildingType::Tower:
        return BuildingSpec{"Torre", 2};
    case BuildingType::IronMine:
        return BuildingSpec{"Mina de Ferro", 2};
    case BuildingType::CoalMine:
        return BuildingSpec{"Mina de Carvao", 2};
    case BuildingType::Steelworks:
        return BuildingSpec{"Siderurgia", 3};
    case BuildingType::CopperMine:
        return BuildingSpec{"Mina de Cobre", 2};
    case BuildingType::ElectronicsFactory:
        return BuildingSpec{"Fabrica de Eletronicos", 3};
    case BuildingType::ToolFactory:
        return BuildingSpec{"Fabrica de Ferramentas", 3};
    case BuildingType::WeaponFactory:
        return BuildingSpec{"Fabrica de Armas", 3};
    case BuildingType::AmmoFactory:
        return BuildingSpec{"Fabrica de Municao", 3};
    case BuildingType::TrainingDummy:
        return BuildingSpec{"Boneco de Treinamento", 1};
    }
    return BuildingSpec{"?", 1};
}

// Combat health given to a construction when it is placed.
[[nodiscard]] constexpr int buildingMaxHealth(BuildingType type)
{
    return type == BuildingType::TrainingDummy ? 100 : 500;
}

// Nominal construction time, in seconds. Zero means the building is placed ready (Command Center).
[[nodiscard]] constexpr float buildingBuildSeconds(BuildingType type)
{
    switch (type) {
    case BuildingType::CommandCenter:
        return 0.0F;
    case BuildingType::Wall:
    case BuildingType::TrainingDummy:
        return 3.0F;
    case BuildingType::House:
    case BuildingType::Generator:
    case BuildingType::Warehouse:
    case BuildingType::Tower:
    case BuildingType::IronMine:
    case BuildingType::CoalMine:
    case BuildingType::CopperMine:
        return 8.0F;
    case BuildingType::Barracks:
    case BuildingType::Cruzador:
    case BuildingType::Steelworks:
    case BuildingType::ElectronicsFactory:
    case BuildingType::ToolFactory:
    case BuildingType::WeaponFactory:
    case BuildingType::AmmoFactory:
        return 12.0F;
    }
    return 0.0F;
}

// The cursor tile is the center of the footprint.
[[nodiscard]] constexpr GridPos footprintOrigin(GridPos anchor, int size)
{
    return GridPos{anchor.x - size / 2, anchor.y - size / 2};
}

} // namespace model
