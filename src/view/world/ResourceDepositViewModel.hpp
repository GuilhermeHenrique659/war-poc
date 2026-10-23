#pragma once

#include "model/WorldMap.hpp"
#include "view/IsoProjection.hpp"
#include "view/StaticSpriteViewModel.hpp"

#include <raylib.h>

#include <array>

namespace view::gameplay {

class ResourceDepositViewModel {
public:
    ResourceDepositViewModel();

    void draw(const model::ResourceDeposit& deposit) const;

    [[nodiscard]] static Rectangle tileDestination(model::GridPos tile)
    {
        return iso::spriteDestinationFromNorth(
            static_cast<float>(tile.x), static_cast<float>(tile.y), Vector2{iso::kTileWidth, iso::kTileHeight});
    }

    [[nodiscard]] static constexpr const char* textureKey(model::ItemId resource) noexcept
    {
        switch (resource) {
        case model::ItemId::Iron: return "deposit/iron";
        case model::ItemId::Coal: return "deposit/coal";
        case model::ItemId::Copper: return "deposit/copper";
        case model::ItemId::Steel:
        case model::ItemId::Electronics:
        case model::ItemId::Tools:
        case model::ItemId::Weapon:
        case model::ItemId::Ammo:
        case model::ItemId::Count: return nullptr;
        }
        return nullptr;
    }

private:
    std::array<StaticSpriteViewModel, 3> sprites_;
};

} // namespace view::gameplay
