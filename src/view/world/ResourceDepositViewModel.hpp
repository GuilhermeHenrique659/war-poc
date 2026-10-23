#pragma once

#include "model/WorldMap.hpp"
#include "view/IsoProjection.hpp"

#include <raylib.h>

namespace view::gameplay {

class ResourceDepositViewModel {
public:
    ResourceDepositViewModel();

    void draw(const model::ResourceDeposit& deposit) const;

    [[nodiscard]] static Rectangle tileDestination(model::GridPos tile)
    {
        const Vector2 top = iso::gridToWorld(tile);
        return Rectangle{top.x - iso::kHalfWidth, top.y, iso::kTileWidth, iso::kTileHeight};
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
        case model::ItemId::Count: return nullptr;
        }
        return nullptr;
    }
};

} // namespace view::gameplay
