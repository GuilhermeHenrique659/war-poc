#include "view/build/WarehouseViewModel.hpp"

#include "view/IsoProjection.hpp"
#include "view/SpriteSheet.hpp"

#include <stdexcept>

namespace view::gameplay {

namespace {

constexpr int kFrameCount = 1;
constexpr Vector2 kFrameSize{1395.0F, 1127.0F};
constexpr double kFramesPerSecond = 1.0;

[[nodiscard]] SpriteSheet loadWarehouseSpriteSheet()
{
    return SpriteSheet{
        "buildings/warehouse", "resource/textures/buildings/warehouse.png", kFrameCount, kFrameSize,
        kFramesPerSecond};
}

} // namespace

WarehouseViewModel::WarehouseViewModel()
    : WarehouseViewModel(loadWarehouseSpriteSheet())
{
}

WarehouseViewModel::WarehouseViewModel(SpriteSheet spriteSheet)
    : sprite_{spriteSheet}
    , frameSize_{spriteSheet.frameSize()}
{
}

void WarehouseViewModel::drawAt(float gridX, float gridY, int size, unsigned char alpha) const
{
    if (size <= 0) {
        throw std::invalid_argument("Warehouse footprint size must be positive");
    }

    const Rectangle destination = iso::spriteDestinationFromSouth(
        gridX, gridY, Vector2{static_cast<float>(size), static_cast<float>(size)}, frameSize_);
    sprite_.drawAt(destination, Color{255, 255, 255, alpha});
}

} // namespace view::gameplay
