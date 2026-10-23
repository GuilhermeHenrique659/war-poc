#include "view/world/ResourceDepositViewModel.hpp"

#include "view/IsoProjection.hpp"

#include <array>
#include <cstddef>
#include <optional>

namespace view::gameplay {

namespace {

struct DepositSprite {
    const char* key;
    const char* path;
    Rectangle source;
};

constexpr std::array<DepositSprite, 3> kResourceSprites{{
    {"deposit/iron", "resource/textures/enviroment/iron.png", {54.0f, 160.0f, 1385.0f, 764.0f}},
    {"deposit/coal", "resource/textures/enviroment/coal.png", {0.0f, 98.0f, 1491.0f, 846.0f}},
    {"deposit/copper", "resource/textures/enviroment/copper.png", {0.0f, 112.0f, 1491.0f, 833.0f}},
}};

[[nodiscard]] std::optional<std::size_t> spriteIndex(model::ItemId resource) noexcept
{
    switch (resource) {
    case model::ItemId::Iron: return 0;
    case model::ItemId::Coal: return 1;
    case model::ItemId::Copper: return 2;
    case model::ItemId::Steel:
    case model::ItemId::Electronics:
    case model::ItemId::Tools:
    case model::ItemId::Weapon:
    case model::ItemId::Ammo:
    case model::ItemId::Count: return std::nullopt;
    }
    return std::nullopt;
}

[[nodiscard]] StaticSpriteViewModel makeSprite(const DepositSprite& descriptor)
{
    return StaticSpriteViewModel{descriptor.key, descriptor.path, descriptor.source,
        static_cast<int>(iso::kTileWidth), static_cast<int>(iso::kTileHeight)};
}

} // namespace

ResourceDepositViewModel::ResourceDepositViewModel()
    : sprites_{
          makeSprite(kResourceSprites[0]), makeSprite(kResourceSprites[1]), makeSprite(kResourceSprites[2])}
{
}

const char* ResourceDepositViewModel::textureKey(model::ItemId resource) noexcept
{
    const std::optional<std::size_t> index = spriteIndex(resource);
    return index ? kResourceSprites[*index].key : nullptr;
}

void ResourceDepositViewModel::draw(const model::ResourceDeposit& deposit) const
{
    const std::optional<std::size_t> index = spriteIndex(deposit.resource);
    if (!index) {
        return;
    }
    const StaticSpriteViewModel& sprite = sprites_[*index];

    for (int y = 0; y < deposit.size; ++y) {
        for (int x = 0; x < deposit.size; ++x) {
            const model::GridPos tile{deposit.origin.x + x, deposit.origin.y + y};
            sprite.drawAt(tileDestination(tile));
        }
    }
}

} // namespace view::gameplay
