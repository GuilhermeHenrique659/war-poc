#pragma once

#include "view/world/WorldObjectViewModel.hpp"

#include <raylib.h>

namespace view::gameplay {

class GroundTileViewModel final : public WorldObjectViewModel {
public:
    GroundTileViewModel();

    void drawAt(float gridX, float gridY) const override;

private:
    Texture2D texture_;
    Vector2 gameSize_;
};

} // namespace view::gameplay
