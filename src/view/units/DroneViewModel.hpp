#pragma once

#include "view/AnimatedSpriteViewModel.hpp"
#include "view/world/WorldObjectViewModel.hpp"

namespace view::gameplay {

class DroneViewModel final : public WorldObjectViewModel {
public:
    DroneViewModel();
    DroneViewModel(SpriteSheet loadedSpriteSheet, SpriteSheet unloadedSpriteSheet);

    void setLoaded(bool loaded) noexcept;
    void drawAt(float gridX, float gridY) const override;

private:
    Vector2 frameSize_{};
    AnimatedSpriteViewModel loadedAnimation_;
    AnimatedSpriteViewModel unloadedAnimation_;
    bool loaded_ = false;
};

} // namespace view::gameplay
