#include "view/build/BarracksViewModel.hpp"

#include "view/IsoProjection.hpp"

#include <stdexcept>
#include <utility>

namespace view::gameplay {

namespace {

constexpr Vector2 kSpriteFrameSize{1536.0F, 1024.0F};
constexpr Rectangle kSpriteSource{0.0F, 0.0F, 1536.0F, 1024.0F};
constexpr int kSpriteWidth = 1536;
constexpr int kSpriteHeight = 1024;

[[nodiscard]] StaticSpriteViewModel loadBarracksSprite()
{
    return StaticSpriteViewModel{
        "buildings/barracks", "resource/textures/buildings/barrack.png", kSpriteSource, kSpriteWidth, kSpriteHeight};
}

} // namespace

BarracksViewModel::BarracksViewModel()
    : BarracksViewModel(loadBarracksSprite())
{
}

BarracksViewModel::BarracksViewModel(StaticSpriteViewModel sprite)
    : sprite_{std::move(sprite)}
{
}

bool BarracksViewModel::handles(model::BuildingType type) noexcept
{
    return type == model::BuildingType::Barracks;
}

void BarracksViewModel::drawAt(float gridX, float gridY, int size, unsigned char alpha) const
{
    if (size <= 0) {
        throw std::invalid_argument("Barracks footprint size must be positive");
    }

    const Rectangle destination = iso::spriteDestinationFromSouth(
        gridX, gridY, Vector2{static_cast<float>(size), static_cast<float>(size)}, kSpriteFrameSize);
    sprite_.drawAt(destination, Color{255, 255, 255, alpha});
}

} // namespace view::gameplay
