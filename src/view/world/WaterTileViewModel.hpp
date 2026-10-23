#pragma once

#include "view/world/WorldObjectViewModel.hpp"

namespace view::gameplay {

class WaterTileViewModel final : public WorldObjectViewModel {
public:
    void drawAt(float gridX, float gridY) const override;
};

} // namespace view::gameplay
