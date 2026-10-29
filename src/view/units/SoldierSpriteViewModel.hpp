#pragma once

#include "view/AnimatedSpriteViewModel.hpp"
#include "view/SpriteSheet.hpp"
#include "view/units/UnitFacing.hpp"

#include <array>
#include <cstddef>
#include <vector>

namespace view {

// Draws a soldier from the eight directional walk sheets in resource/textures/units/soldier/.
// Each sheet is cropped and normalized to the same rendered height so the eight directions keep
// a consistent size even though the source art does not.
class SoldierSpriteViewModel {
public:
    static constexpr std::size_t kFacingCount = 8;

    SoldierSpriteViewModel();
    explicit SoldierSpriteViewModel(std::array<SpriteSheet, kFacingCount> sheets);

    void drawAt(float gridX, float gridY, UnitFacing facing) const;

private:
    struct DirectionAnimation {
        AnimatedSpriteViewModel animation;
        Vector2 frameSize;
    };

    std::vector<DirectionAnimation> directions_;
};

} // namespace view
