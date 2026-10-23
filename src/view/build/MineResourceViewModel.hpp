#pragma once

#include "model/build/BuildingType.hpp"
#include "view/AnimatedSpriteViewModel.hpp"

#include <array>

namespace view::gameplay {

class MineResourceViewModel {
public:
    MineResourceViewModel();

    [[nodiscard]] static bool handles(model::BuildingType type) noexcept;
    void drawAt(model::BuildingType type, float gridX, float gridY, int size, unsigned char alpha = 255) const;

private:
    std::array<AnimatedSpriteViewModel, 3> animations_;
};

} // namespace view::gameplay
