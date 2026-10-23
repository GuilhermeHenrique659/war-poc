#include "view/world/WaterTileViewModel.hpp"

#include "view/IsoProjection.hpp"

#include <raylib.h>

namespace view::gameplay {

namespace {

constexpr const char* kTextureKey = "terrain/water";
constexpr const char* kTexturePath = "resource/textures/enviroment/water.png";
constexpr int kFrameColumns = 4;
constexpr int kFrameRows = 4;
constexpr int kFrameCount = kFrameColumns * kFrameRows;
constexpr Vector2 kFrameSize{443.5F, 221.75F};
constexpr double kFramesPerSecond = 1.0 / 0.12;
constexpr float kTileOverlap = 1.0F;

} // namespace

WaterTileViewModel::WaterTileViewModel()
    : animation_(SpriteSheet{kTextureKey, kTexturePath, kFrameCount, kFrameSize, kFramesPerSecond})
    , gameSize_{iso::kTileWidth, iso::kTileHeight}
{
}

void WaterTileViewModel::drawAt(float gridX, float gridY) const
{
    const Rectangle destination = iso::spriteDestinationFromNorth(
        gridX, gridY, gameSize_, kTileOverlap, kTileOverlap);
    animation_.drawAt(destination);
}

} // namespace view::gameplay
