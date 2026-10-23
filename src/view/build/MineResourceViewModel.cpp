#include "view/build/MineResourceViewModel.hpp"

#include "view/IsoProjection.hpp"

#include <cstddef>

namespace view::gameplay {

namespace {

constexpr int kFrameCount = 8;
constexpr Vector2 kFrameSize{443.5F, 443.5F};
constexpr double kFramesPerSecond = 8.0;

[[nodiscard]] SpriteSheet loadMineSpriteSheet(const char* key, const char* path)
{
    return SpriteSheet{key, path, kFrameCount, kFrameSize, kFramesPerSecond};
}

} // namespace

MineResourceViewModel::MineResourceViewModel()
    : animations_{AnimatedSpriteViewModel{loadMineSpriteSheet(
                    "buildings/iron_mine", "resource/textures/buildings/iron_mine.png")},
          AnimatedSpriteViewModel{loadMineSpriteSheet(
              "buildings/coal_mine", "resource/textures/buildings/coal_mine.png")},
          AnimatedSpriteViewModel{loadMineSpriteSheet(
              "buildings/copper_mine", "resource/textures/buildings/copper_mine.png")}}
{
}

bool MineResourceViewModel::handles(model::BuildingType type) noexcept
{
    switch (type) {
    case model::BuildingType::IronMine:
    case model::BuildingType::CoalMine:
    case model::BuildingType::CopperMine: return true;
    default: return false;
    }
}

void MineResourceViewModel::drawAt(
    model::BuildingType type, float gridX, float gridY, int size, unsigned char alpha) const
{
    std::size_t animationIndex = 0;
    switch (type) {
    case model::BuildingType::IronMine: animationIndex = 0; break;
    case model::BuildingType::CoalMine: animationIndex = 1; break;
    case model::BuildingType::CopperMine: animationIndex = 2; break;
    default: return;
    }

    const Rectangle destination = iso::spriteDestinationFromSouth(
        gridX, gridY, Vector2{static_cast<float>(size), static_cast<float>(size)}, kFrameSize);
    animations_[animationIndex].drawAt(destination, Color{255, 255, 255, alpha});
}

} // namespace view::gameplay
