#pragma once

#include "model/units/UnitType.hpp"
#include "view/units/UnitFacing.hpp"

#include <raylib.h>

namespace view {

class SoldierSpriteViewModel;

// Placeholder art for units that do not have a sprite yet: they are drawn from small isometric boxes.
[[nodiscard]] Color unitColor(model::UnitType type);

// All draw in world space (inside BeginMode2D). Positions are the unit's center in continuous
// grid coordinates; size is its square footprint in tiles per side. Soldiers use their directional
// sprites when a sprite view model is supplied; the other unit types keep the placeholder art.
void drawUnit(model::UnitType type, float gx, float gy, int size,
    UnitFacing facing = UnitFacing::South, const SoldierSpriteViewModel* soldierSprite = nullptr);
void drawUnitSelectionRing(float gx, float gy, int size);
void drawUnitHealthBar(float gx, float gy, int size, float ratio);

} // namespace view
