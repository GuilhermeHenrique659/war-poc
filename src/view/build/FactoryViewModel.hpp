#pragma once

#include "model/build/BuildingType.hpp"
#include "view/StaticSpriteViewModel.hpp"

#include <array>

namespace view::gameplay {

class FactoryViewModel {
public:
    FactoryViewModel();
    explicit FactoryViewModel(std::array<StaticSpriteViewModel, 4> sprites);

    [[nodiscard]] static bool handles(model::BuildingType type) noexcept;
    void drawAt(model::BuildingType type, float gridX, float gridY, int size, unsigned char alpha = 255) const;

private:
    std::array<StaticSpriteViewModel, 4> sprites_;
};

} // namespace view::gameplay
