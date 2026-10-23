#include "view/world/WaterTileViewModel.hpp"

#include "view/IsoProjection.hpp"
#include "view/TextureManager.hpp"

#include <raylib.h>

namespace view::gameplay {

namespace {

constexpr const char* kTextureKey = "terrain/water";
constexpr const char* kTexturePath = "resource/textures/enviroment/water.png";
constexpr int kFrameColumns = 4;
constexpr int kFrameRows = 4;
constexpr double kFrameDuration = 0.12;
constexpr float kTileOverlap = 1.0F;

[[nodiscard]] Texture2D loadWaterTexture()
{
    TextureManager& textures = TextureManager::instance();
    textures.load(kTextureKey, kTexturePath);
    const Texture2D texture = textures.get(kTextureKey);
    SetTextureFilter(texture, TEXTURE_FILTER_POINT);
    return texture;
}

} // namespace

WaterTileViewModel::WaterTileViewModel()
    : animation_(loadWaterTexture(), kFrameColumns, kFrameRows, kFrameDuration)
    , gameSize_{iso::kTileWidth, iso::kTileHeight}
{
}

void WaterTileViewModel::drawAt(float gridX, float gridY) const
{
    const Vector2 north = iso::gridToWorld(gridX, gridY);
    const Rectangle destination{north.x - gameSize_.x / 2.0F - kTileOverlap,
        north.y - kTileOverlap, gameSize_.x + 2.0F * kTileOverlap, gameSize_.y + 2.0F * kTileOverlap};
    animation_.drawAt(destination);
}

} // namespace view::gameplay
