#pragma once

namespace view {

// Placeholder art for the Combined unit: a mech that scales with its footprint (tiles per side).
// Draws in world space (inside BeginMode2D); the position is the unit's center in grid coordinates.
void drawMech(float gx, float gy, int size);

} // namespace view
