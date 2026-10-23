#pragma once

#include "view/AnimatedSpriteViewModel.hpp"
#include "view/world/WorldObjectViewModel.hpp"

namespace view::gameplay {

class DroneViewModel final : public WorldObjectViewModel {
public:
    DroneViewModel();
    explicit DroneViewModel(SpriteSheet spriteSheet);

    void drawAt(float gridX, float gridY) const override;

private:
    AnimatedSpriteViewModel animation_;
    Vector2 frameSize_{};
};

} // namespace view::gameplay
