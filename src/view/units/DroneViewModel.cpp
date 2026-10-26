#include "view/units/DroneViewModel.hpp"

#include "view/IsoProjection.hpp"
#include "view/SpriteSheet.hpp"

#include <utility>

namespace view::gameplay {

namespace {

constexpr int kFrameCount = 8;
constexpr Vector2 kFrameSize{443.5F, 443.5F};
constexpr double kFramesPerSecond = 8.0;

[[nodiscard]] SpriteSheet loadDroneLoadedSpriteSheet()
{
    return SpriteSheet{
        "units/drone_loaded", "resource/textures/units/drone_loaded.png", kFrameCount, kFrameSize, kFramesPerSecond};
}

[[nodiscard]] SpriteSheet loadDroneUnloadedSpriteSheet()
{
    return SpriteSheet{"units/drone_unloaded", "resource/textures/units/drone_unloaded.png", kFrameCount,
        kFrameSize, kFramesPerSecond};
}

} // namespace

DroneViewModel::DroneViewModel()
    : DroneViewModel(loadDroneLoadedSpriteSheet(), loadDroneUnloadedSpriteSheet())
{
}

DroneViewModel::DroneViewModel(SpriteSheet loadedSpriteSheet, SpriteSheet unloadedSpriteSheet)
    : frameSize_{loadedSpriteSheet.frameSize()}
    , loadedAnimation_{std::move(loadedSpriteSheet)}
    , unloadedAnimation_{std::move(unloadedSpriteSheet)}
{
}

void DroneViewModel::setLoaded(bool loaded) noexcept
{
    loaded_ = loaded;
}

void DroneViewModel::setTint(Color tint) noexcept
{
    tint_ = tint;
}

void DroneViewModel::drawAt(float gridX, float gridY) const
{
    const Rectangle destination = iso::spriteDestinationFromSouth(
        gridX - 1.0F, gridY - 1.0F, Vector2{1.0F, 1.0F}, frameSize_);
    if (loaded_) {
        loadedAnimation_.drawAt(destination, tint_);
        return;
    }
    unloadedAnimation_.drawAt(destination, tint_);
}

} // namespace view::gameplay
