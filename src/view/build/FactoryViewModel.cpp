#include "view/build/FactoryViewModel.hpp"

#include "view/IsoProjection.hpp"

#include <cstddef>
#include <stdexcept>
#include <utility>

namespace view::gameplay {

namespace {

constexpr Vector2 kSpriteFrameSize{1536.0F, 1024.0F};
constexpr Rectangle kSpriteSource{0.0F, 0.0F, 1536.0F, 1024.0F};
constexpr int kSpriteWidth = 1536;
constexpr int kSpriteHeight = 1024;

[[nodiscard]] StaticSpriteViewModel loadFactorySprite(const char* key, const char* path)
{
    return StaticSpriteViewModel{key, path, kSpriteSource, kSpriteWidth, kSpriteHeight};
}

[[nodiscard]] std::size_t spriteIndex(model::BuildingType type) noexcept
{
    switch (type) {
    case model::BuildingType::ElectronicsFactory: return 0;
    case model::BuildingType::ToolFactory: return 1;
    case model::BuildingType::WeaponFactory: return 2;
    case model::BuildingType::AmmoFactory: return 3;
    default: return 0;
    }
}

} // namespace

FactoryViewModel::FactoryViewModel()
    : sprites_{
          loadFactorySprite("buildings/electronics_factory", "resource/textures/buildings/eletronic_factory.png"),
          loadFactorySprite("buildings/tool_factory", "resource/textures/buildings/tool_factory.png"),
          loadFactorySprite("buildings/weapon_factory", "resource/textures/buildings/weapon_factory.png"),
          loadFactorySprite("buildings/ammo_factory", "resource/textures/buildings/ammo_factory.png")}
{
}

FactoryViewModel::FactoryViewModel(std::array<StaticSpriteViewModel, 4> sprites)
    : sprites_{std::move(sprites)}
{
}

bool FactoryViewModel::handles(model::BuildingType type) noexcept
{
    switch (type) {
    case model::BuildingType::ElectronicsFactory:
    case model::BuildingType::ToolFactory:
    case model::BuildingType::WeaponFactory:
    case model::BuildingType::AmmoFactory: return true;
    default: return false;
    }
}

void FactoryViewModel::drawAt(
    model::BuildingType type, float gridX, float gridY, int size, unsigned char alpha) const
{
    if (!handles(type)) {
        return;
    }
    if (size <= 0) {
        throw std::invalid_argument("Factory footprint size must be positive");
    }

    const Rectangle destination = iso::spriteDestinationFromSouth(
        gridX, gridY, Vector2{static_cast<float>(size), static_cast<float>(size)}, kSpriteFrameSize);
    sprites_[spriteIndex(type)].drawAt(destination, Color{255, 255, 255, alpha});
}

} // namespace view::gameplay
