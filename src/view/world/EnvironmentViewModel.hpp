#pragma once

#include "model/WorldMap.hpp"
#include "view/world/GroundTileViewModel.hpp"
#include "view/world/WaterTileViewModel.hpp"
#include "view/world/WorldObjectViewModel.hpp"

namespace view::gameplay {

class EnvironmentViewModel {
public:
    void drawTile(model::Terrain terrain, float gridX, float gridY) const;

private:
    GroundTileViewModel ground_;
    WaterTileViewModel water_;
};

} // namespace view::gameplay
