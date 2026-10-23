#pragma once

#include "model/build/BuildingType.hpp"
#include "view/AnimatedSpriteViewModel.hpp"

namespace view::gameplay {

class SteelworksViewModel {
public:
    SteelworksViewModel();
    explicit SteelworksViewModel(SpriteSheet spriteSheet);

    [[nodiscard]] static bool handles(model::BuildingType type) noexcept;
    void drawAt(float gridX, float gridY, int size, unsigned char alpha = 255) const;

private:
    AnimatedSpriteViewModel animation_;
    Vector2 frameSize_{};
};

} // namespace view::gameplay
