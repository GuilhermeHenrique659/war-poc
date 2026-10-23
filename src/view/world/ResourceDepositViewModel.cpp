#include "view/world/ResourceDepositViewModel.hpp"

#include "view/IsoProjection.hpp"

#include <cstddef>

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

[[nodiscard]] std::size_t spriteIndex(model::ItemId resource)
{
    switch (resource) {
    case model::ItemId::Iron: return 0;
    case model::ItemId::Coal: return 1;
    case model::ItemId::Copper: return 2;
    case model::ItemId::Steel:
    case model::ItemId::Electronics:
    case model::ItemId::Tools:
    case model::ItemId::Count: return 0;
    }
    return 0;
}

} // namespace

ResourceDepositViewModel::ResourceDepositViewModel()
    : sprites_{StaticSpriteViewModel{kIronTextureKey, kIronTexturePath, kIronTextureSource,
                   static_cast<int>(iso::kTileWidth), static_cast<int>(iso::kTileHeight)},
          StaticSpriteViewModel{kCoalTextureKey, kCoalTexturePath, kCoalTextureSource,
              static_cast<int>(iso::kTileWidth), static_cast<int>(iso::kTileHeight)},
          StaticSpriteViewModel{kCopperTextureKey, kCopperTexturePath, kCopperTextureSource,
              static_cast<int>(iso::kTileWidth), static_cast<int>(iso::kTileHeight)}}
{
}

void ResourceDepositViewModel::draw(const model::ResourceDeposit& deposit) const
{
    const char* key = textureKey(deposit.resource);
    if (key == nullptr) {
        return;
    }
    const StaticSpriteViewModel& sprite = sprites_[spriteIndex(deposit.resource)];

    for (int y = 0; y < deposit.size; ++y) {
        for (int x = 0; x < deposit.size; ++x) {
            const model::GridPos tile{deposit.origin.x + x, deposit.origin.y + y};
            sprite.drawAt(tileDestination(tile));
        }
    }
}

} // namespace view::gameplay
