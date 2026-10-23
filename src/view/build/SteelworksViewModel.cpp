#include "view/build/SteelworksViewModel.hpp"

#include "view/IsoProjection.hpp"

#include <stdexcept>

namespace view::gameplay {

namespace {

<<<<<<< Updated upstream
constexpr int kFrameCount = 21;
constexpr Vector2 kFrameSize{306.0F, 238.0F};
constexpr double kFrameDurationSeconds = 0.125;
constexpr double kFramesPerSecond = 1.0 / kFrameDurationSeconds;
=======
constexpr int kFrameCount = 8;
constexpr Vector2 kFrameSize{443.5F, 443.5F};
constexpr double kFramesPerSecond = 8.0;
>>>>>>> Stashed changes

[[nodiscard]] SpriteSheet loadSteelworksSpriteSheet()
{
    return SpriteSheet{"buildings/steelworks", "resource/textures/buildings/steelworks.png",
        kFrameCount, kFrameSize, kFramesPerSecond};
}

} // namespace

SteelworksViewModel::SteelworksViewModel()
    : SteelworksViewModel(loadSteelworksSpriteSheet())
{
}

SteelworksViewModel::SteelworksViewModel(SpriteSheet spriteSheet)
    : animation_{spriteSheet}
    , frameSize_{spriteSheet.frameSize()}
{
}

bool SteelworksViewModel::handles(model::BuildingType type) noexcept
{
    return type == model::BuildingType::Steelworks;
}

void SteelworksViewModel::drawAt(float gridX, float gridY, int size, unsigned char alpha) const
{
    if (size <= 0) {
        throw std::invalid_argument("Steelworks footprint size must be positive");
    }

    const Rectangle destination = iso::spriteDestinationFromSouth(
        gridX, gridY, Vector2{static_cast<float>(size), static_cast<float>(size)}, frameSize_);
    animation_.drawAt(destination, Color{255, 255, 255, alpha});
}

} // namespace view::gameplay
