#pragma once

#include "view/StaticSpriteViewModel.hpp"
#include "view/world/WorldObjectViewModel.hpp"

#include <raylib.h>

namespace view::gameplay {

class GroundTileViewModel final : public WorldObjectViewModel {
public:
    GroundTileViewModel();

    void drawAt(float gridX, float gridY) const override;

private:
    StaticSpriteViewModel sprite_;
    Vector2 gameSize_;
};

} // namespace view::gameplay
