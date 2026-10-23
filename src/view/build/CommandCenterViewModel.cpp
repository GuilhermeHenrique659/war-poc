#include "view/build/CommandCenterViewModel.hpp"

#include "view/IsoProjection.hpp"
#include "view/SpriteSheet.hpp"

#include <stdexcept>

namespace view::gameplay {

namespace {

constexpr int kFrameCount = 1;
constexpr Vector2 kFrameSize{1536.0F, 1024.0F};
constexpr double kFramesPerSecond = 1.0;

[[nodiscard]] SpriteSheet loadCommandCenterSpriteSheet()
{
    return SpriteSheet{
        "buildings/command_center", "resource/textures/buildings/command_center.png", kFrameCount, kFrameSize,
        kFramesPerSecond};
}

} // namespace

CommandCenterViewModel::CommandCenterViewModel()
    : CommandCenterViewModel(loadCommandCenterSpriteSheet())
{
}

CommandCenterViewModel::CommandCenterViewModel(SpriteSheet spriteSheet)
    : animation_{spriteSheet}
    , frameSize_{spriteSheet.frameSize()}
{
}

void CommandCenterViewModel::drawAt(float gridX, float gridY, int size, unsigned char alpha) const
{
    if (size <= 0) {
        throw std::invalid_argument("Command center footprint size must be positive");
    }

    const Rectangle destination = iso::spriteDestinationFromSouth(
        gridX, gridY, Vector2{static_cast<float>(size), static_cast<float>(size)}, frameSize_);
    animation_.drawAt(destination, Color{255, 255, 255, alpha});
}

} // namespace view::gameplay
