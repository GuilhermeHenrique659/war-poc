#include "view/world/WaterTileViewModel.hpp"

#include "view/IsoShapes.hpp"

#include <raylib.h>

namespace view::gameplay {

namespace {

constexpr Color kWaterColor{42, 105, 156, 255};

} // namespace

void WaterTileViewModel::drawAt(float gridX, float gridY) const
{
    iso::drawFootprint(gridX, gridY, 1.0f, 1.0f, kWaterColor, Color{0, 0, 0, 0});
}

} // namespace view::gameplay
