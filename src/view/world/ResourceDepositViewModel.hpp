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

    [[nodiscard]] static const char* textureKey(model::ItemId resource) noexcept;

private:
    std::array<StaticSpriteViewModel, 3> sprites_;
};

} // namespace view::gameplay
