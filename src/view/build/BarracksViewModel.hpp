#pragma once

#include "model/build/BuildingType.hpp"
#include "view/StaticSpriteViewModel.hpp"

namespace view::gameplay {

class BarracksViewModel {
public:
    BarracksViewModel();
    explicit BarracksViewModel(StaticSpriteViewModel sprite);

    [[nodiscard]] static bool handles(model::BuildingType type) noexcept;
    void drawAt(float gridX, float gridY, int size, unsigned char alpha = 255) const;

private:
    StaticSpriteViewModel sprite_;
};

} // namespace view::gameplay
