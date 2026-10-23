#include "view/build/BuildingSpriteViewModel.hpp"

#include "view/IsoProjection.hpp"
#include "view/SpriteSheet.hpp"

#include <stdexcept>

namespace view::gameplay {

namespace {

constexpr Vector2 kCommandCenterFrameSize{1536.0F, 1024.0F};
constexpr Vector2 kWarehouseFrameSize{1395.0F, 1127.0F};
constexpr Vector2 kSteelworksFrameSize{443.5F, 443.5F};
constexpr Vector2 kBarracksFrameSize{1536.0F, 1024.0F};

} // namespace

BuildingSpriteViewModel::BuildingSpriteViewModel(SpriteSheet sheet)
    : animation_{sheet}
    , frameSize_{sheet.frameSize()}
{
}

void BuildingSpriteViewModel::drawAt(float gridX, float gridY, int size, unsigned char alpha) const
{
    if (size <= 0) {
        throw std::invalid_argument("Building footprint size must be positive");
    }

    const Rectangle destination = iso::spriteDestinationFromSouth(
        gridX, gridY, Vector2{static_cast<float>(size), static_cast<float>(size)}, frameSize_);
    animation_.drawAt(destination, Color{255, 255, 255, alpha});
}

BuildingSpriteViewModel loadCommandCenterViewModel()
{
    return BuildingSpriteViewModel{SpriteSheet{"buildings/command_center",
        "resource/textures/buildings/command_center.png", 1, kCommandCenterFrameSize, 1.0}};
}

BuildingSpriteViewModel loadWarehouseViewModel()
{
    return BuildingSpriteViewModel{
        SpriteSheet{"buildings/warehouse", "resource/textures/buildings/warehouse.png", 1, kWarehouseFrameSize, 1.0}};
}

BuildingSpriteViewModel loadSteelworksViewModel()
{
    return BuildingSpriteViewModel{
        SpriteSheet{"buildings/steelworks", "resource/textures/buildings/steelworks.png", 8, kSteelworksFrameSize, 8.0}};
}

BuildingSpriteViewModel loadBarracksViewModel()
{
    return BuildingSpriteViewModel{
        SpriteSheet{"buildings/barracks", "resource/textures/buildings/barrack.png", 1, kBarracksFrameSize, 1.0}};
}

} // namespace view::gameplay
