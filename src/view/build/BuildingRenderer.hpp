#pragma once

#include "model/build/BuildingType.hpp"
#include "model/GridPos.hpp"

#include <raylib.h>

namespace view {

// Placeholder art (no assets yet): buildings are drawn from isometric boxes.
[[nodiscard]] Color buildingColor(model::BuildingType type);

// Draws in world space; call inside BeginMode2D.
void drawBuilding(model::BuildingType type, model::GridPos origin, int size, unsigned char alpha = 255);

} // namespace view
