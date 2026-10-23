#include "view/units/DroneViewModel.hpp"

#include "view/IsoProjection.hpp"
#include "view/SpriteSheet.hpp"

#include <stdexcept>

namespace view::gameplay {

namespace {

constexpr int kFrameCount = 8;
constexpr Vector2 kFrameSize{443.5F, 443.5F};
constexpr double kFramesPerSecond = 8.0;

[[nodiscard]] SpriteSheet loadDroneSpriteSheet()
{
    return SpriteSheet{"units/drone", "resource/textures/units/drone.png", kFrameCount, kFrameSize, kFramesPerSecond};
}

} // namespace

DroneViewModel::DroneViewModel()
    : DroneViewModel(loadDroneSpriteSheet())
{
}

DroneViewModel::DroneViewModel(SpriteSheet spriteSheet)
    : animation_{spriteSheet}
    , frameSize_{spriteSheet.frameSize()}
{
}

void DroneViewModel::drawAt(float gridX, float gridY) const
{
    const Rectangle destination = iso::spriteDestinationFromSouth(
        gridX - 1.0F, gridY - 1.0F, Vector2{1.0F, 1.0F}, frameSize_);
    animation_.drawAt(destination);
}

} // namespace view::gameplay
