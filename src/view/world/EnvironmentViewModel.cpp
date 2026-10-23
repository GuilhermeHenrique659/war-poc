#include "view/world/EnvironmentViewModel.hpp"

namespace view::gameplay {

void EnvironmentViewModel::drawTile(model::Terrain terrain, float gridX, float gridY) const
{
    switch (terrain) {
    case model::Terrain::Water:
        water_.drawAt(gridX, gridY);
        return;
    case model::Terrain::Plain:
    case model::Terrain::Blocked:
        ground_.drawAt(gridX, gridY);
        return;
    }
}

} // namespace view::gameplay
