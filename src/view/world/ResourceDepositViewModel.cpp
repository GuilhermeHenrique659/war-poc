#include "view/world/ResourceDepositViewModel.hpp"

#include "view/IsoProjection.hpp"
#include "view/TextureManager.hpp"

namespace view::gameplay {

namespace {

constexpr const char* kIronTextureKey = "deposit/iron";
constexpr const char* kIronTexturePath = "resource/textures/enviroment/iron.png";
constexpr Rectangle kIronTextureSource{54.0f, 160.0f, 1385.0f, 764.0f};
constexpr const char* kCoalTextureKey = "deposit/coal";
constexpr const char* kCoalTexturePath = "resource/textures/enviroment/coal.png";
constexpr Rectangle kCoalTextureSource{0.0f, 98.0f, 1491.0f, 846.0f};
constexpr const char* kCopperTextureKey = "deposit/copper";
constexpr const char* kCopperTexturePath = "resource/textures/enviroment/copper.png";
constexpr Rectangle kCopperTextureSource{0.0f, 112.0f, 1491.0f, 833.0f};

void loadTileTexture(const char* key, const char* path, Rectangle source)
{
    TextureManager::instance().loadResized(
        key, path, source, static_cast<int>(iso::kTileWidth), static_cast<int>(iso::kTileHeight));
    SetTextureFilter(TextureManager::instance().get(key), TEXTURE_FILTER_POINT);
}

} // namespace

ResourceDepositViewModel::ResourceDepositViewModel()
{
    loadTileTexture(kIronTextureKey, kIronTexturePath, kIronTextureSource);
    loadTileTexture(kCoalTextureKey, kCoalTexturePath, kCoalTextureSource);
    loadTileTexture(kCopperTextureKey, kCopperTexturePath, kCopperTextureSource);
}

void ResourceDepositViewModel::draw(const model::ResourceDeposit& deposit) const
{
    const char* key = textureKey(deposit.resource);
    if (key == nullptr) {
        return;
    }
    const Texture2D& texture = TextureManager::instance().get(key);

    for (int y = 0; y < deposit.size; ++y) {
        for (int x = 0; x < deposit.size; ++x) {
            const model::GridPos tile{deposit.origin.x + x, deposit.origin.y + y};
            DrawTexturePro(texture,
                Rectangle{0.0f, 0.0f, static_cast<float>(texture.width), static_cast<float>(texture.height)},
                tileDestination(tile), Vector2{0.0f, 0.0f}, 0.0f, WHITE);
        }
    }
}

} // namespace view::gameplay
