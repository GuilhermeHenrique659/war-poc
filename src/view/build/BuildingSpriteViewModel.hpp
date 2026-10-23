#pragma once

#include "view/AnimatedSpriteViewModel.hpp"

#include <raylib.h>

namespace view::gameplay {

// A building drawn from a single sprite, anchored at the south edge of its square footprint.
// It backs every building whose look is a single texture (animated or one frame).
class BuildingSpriteViewModel {
public:
    explicit BuildingSpriteViewModel(SpriteSheet sheet);

    void drawAt(float gridX, float gridY, int size, unsigned char alpha = 255) const;

private:
    AnimatedSpriteViewModel animation_;
    Vector2 frameSize_;
};

// Standard single-sprite buildings. Each loads its texture from the resources directory.
[[nodiscard]] BuildingSpriteViewModel loadCommandCenterViewModel();
[[nodiscard]] BuildingSpriteViewModel loadWarehouseViewModel();
[[nodiscard]] BuildingSpriteViewModel loadSteelworksViewModel();
[[nodiscard]] BuildingSpriteViewModel loadBarracksViewModel();

} // namespace view::gameplay
