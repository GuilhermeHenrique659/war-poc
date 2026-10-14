#pragma once

#include "model/UnitType.hpp"

#include <raylib.h>

namespace view {

// Placeholder art (no assets yet): units are drawn from small isometric boxes.
[[nodiscard]] Color unitColor(model::UnitType type);

// All draw in world space (inside BeginMode2D). Positions are the unit's center in continuous
// grid coordinates; size is its square footprint in tiles per side.
void drawUnit(model::UnitType type, float gx, float gy, int size);
void drawUnitSelectionRing(float gx, float gy, int size);
void drawUnitHealthBar(float gx, float gy, int size, float ratio);

} // namespace view
