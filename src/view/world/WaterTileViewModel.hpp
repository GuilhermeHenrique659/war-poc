#pragma once

#include "view/world/WorldObjectViewModel.hpp"
#include "view/AnimatedSpriteViewModel.hpp"

#include <raylib.h>

namespace view::gameplay {

class WaterTileViewModel final : public WorldObjectViewModel {
public:
    WaterTileViewModel();

    void drawAt(float gridX, float gridY) const override;

private:
    AnimatedSpriteViewModel animation_;
    Vector2 gameSize_;
};

} // namespace view::gameplay
