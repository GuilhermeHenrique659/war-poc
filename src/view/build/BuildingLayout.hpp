#pragma once

#include "model/GridPos.hpp"
#include "model/build/BuildingType.hpp"
#include "view/IsoProjection.hpp"

#include <raylib.h>

// Shared building geometry: the view draws the sprites with these heights and the controller
// picks buildings with the same area, so clicking the visible sprite always selects it.
namespace view::build_layout {

// How far the procedural sprite rises above its ground footprint, in world pixels.
[[nodiscard]] constexpr float spriteHeight(model::BuildingType type)
{
    switch (type) {
    case model::BuildingType::CommandCenter:
        return 102.0f;
    case model::BuildingType::Wall:
        return 29.0f;
    case model::BuildingType::Tower:
        return 36.0f;
    case model::BuildingType::TrainingDummy:
        return 38.0f;
    case model::BuildingType::House:
    case model::BuildingType::Generator:
    case model::BuildingType::Warehouse:
    case model::BuildingType::Barracks:
    case model::BuildingType::Cruzador:
    case model::BuildingType::IronMine:
    case model::BuildingType::CoalMine:
    case model::BuildingType::CopperMine:
    case model::BuildingType::Steelworks:
    case model::BuildingType::ElectronicsFactory:
    case model::BuildingType::ToolFactory:
    case model::BuildingType::WeaponFactory:
    case model::BuildingType::AmmoFactory:
        return 24.0f;
    }
    return 24.0f;
}

// True when `world` (world pixels) hits the building: either its ground footprint or any part
// of the sprite raised above it.
[[nodiscard]] inline bool contains(model::BuildingType type, model::GridPos origin, int size, Vector2 world)
{
    const auto insideFootprint = [origin, size](model::GridPos tile) {
        return tile.x >= origin.x && tile.x < origin.x + size && tile.y >= origin.y
            && tile.y < origin.y + size;
    };

    if (insideFootprint(iso::worldToGrid(world))) {
        return true;
    }
    // The sprite is the building extruded upwards, so look at which tile lies under the same
    // point lowered by the sprite height.
    return insideFootprint(iso::worldToGrid(Vector2{world.x, world.y + spriteHeight(type)}));
}

} // namespace view::build_layout
