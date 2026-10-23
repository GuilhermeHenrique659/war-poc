#include "view/world/GroundTileViewModel.hpp"

#include "view/IsoProjection.hpp"
#include "view/TextureManager.hpp"

namespace view::gameplay {

namespace {

constexpr const char* kTextureKey = "terrain/ermo";
constexpr const char* kTexturePath = "resource/textures/enviroment/ermo_orginal.png";
constexpr Rectangle kTextureSource{85.0f, 50.0f, 1621.0f, 796.0f};
constexpr float kTileOverlap = 1.0f;

} // namespace

GroundTileViewModel::GroundTileViewModel()
    : gameSize_{iso::kTileWidth, iso::kTileHeight}
{
    TextureManager::instance().loadResized(
        kTextureKey, kTexturePath, kTextureSource, static_cast<int>(gameSize_.x), static_cast<int>(gameSize_.y));
    texture_ = TextureManager::instance().get(kTextureKey);
    SetTextureFilter(texture_, TEXTURE_FILTER_POINT);
}

void GroundTileViewModel::drawAt(float gridX, float gridY) const
{
    const Vector2 north = iso::gridToWorld(gridX, gridY);
    const Rectangle destination{
        north.x - gameSize_.x / 2.0f - kTileOverlap,
        north.y - kTileOverlap,
        gameSize_.x + 2.0f * kTileOverlap,
        gameSize_.y + 2.0f * kTileOverlap
    };
    DrawTexturePro(texture_, Rectangle{0.0f, 0.0f, gameSize_.x, gameSize_.y}, destination,
        Vector2{0.0f, 0.0f}, 0.0f, WHITE);
}

} // namespace view::gameplay
