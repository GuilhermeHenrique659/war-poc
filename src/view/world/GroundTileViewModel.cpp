#include "view/world/GroundTileViewModel.hpp"

#include "view/IsoProjection.hpp"

namespace view::gameplay {

namespace {

constexpr const char* kTextureKey = "terrain/ermo";
constexpr const char* kTexturePath = "resource/textures/enviroment/ermo_orginal.png";
constexpr Rectangle kTextureSource{85.0f, 50.0f, 1621.0f, 796.0f};
constexpr float kTileOverlap = 1.0f;

} // namespace

GroundTileViewModel::GroundTileViewModel()
    : sprite_(kTextureKey, kTexturePath, kTextureSource,
          static_cast<int>(iso::kTileWidth), static_cast<int>(iso::kTileHeight))
    , gameSize_{iso::kTileWidth, iso::kTileHeight}
{
}

void GroundTileViewModel::drawAt(float gridX, float gridY) const
{
    const Rectangle destination = iso::spriteDestinationFromNorth(
        gridX, gridY, gameSize_, kTileOverlap, kTileOverlap);
    sprite_.drawAt(destination);
}

} // namespace view::gameplay
