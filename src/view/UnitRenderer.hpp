#pragma once

#include "model/UnitType.hpp"

#include <raylib.h>

namespace view {

// Placeholder art (no assets yet): units are drawn from small isometric boxes.
[[nodiscard]] Color unitColor(model::UnitType type);

// All draw in world space (inside BeginMode2D). Positions are continuous grid coordinates.
void drawUnit(model::UnitType type, float gx, float gy);
void drawUnitSelectionRing(float gx, float gy);
void drawUnitHealthBar(float gx, float gy, float ratio);

} // namespace view
