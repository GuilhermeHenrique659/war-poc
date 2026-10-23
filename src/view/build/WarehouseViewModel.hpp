#pragma once

#include "view/AnimatedSpriteViewModel.hpp"

#include <raylib.h>

namespace view::gameplay {

class WarehouseViewModel {
public:
    WarehouseViewModel();
    explicit WarehouseViewModel(SpriteSheet spriteSheet);

    void drawAt(float gridX, float gridY, int size, unsigned char alpha = 255) const;

private:
    AnimatedSpriteViewModel sprite_;
    Vector2 frameSize_{};
};

} // namespace view::gameplay
