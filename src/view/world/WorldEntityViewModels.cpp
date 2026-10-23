#include "view/world/WorldEntityViewModels.hpp"

#include "view/build/BuildingRenderer.hpp"
#include "view/IsoProjection.hpp"
#include "view/IsoShapes.hpp"
#include "view/player/PlayerRenderer.hpp"
#include "view/units/UnitRenderer.hpp"

#include <raylib.h>

namespace view::gameplay {

namespace {

constexpr Color kDroneColor{70, 220, 230, 255};

} // namespace

BuildingViewModel::BuildingViewModel(model::BuildingType type, int size)
    : type_(type)
    , size_(size)
{
}

void BuildingViewModel::drawAt(float gridX, float gridY) const
{
    drawBuilding(type_, model::GridPos{static_cast<int>(gridX), static_cast<int>(gridY)}, size_);
}

UnitViewModel::UnitViewModel(model::UnitType type, int size, int health, int maxHealth)
    : type_(type)
    , size_(size)
    , health_(health)
    , maxHealth_(maxHealth)
{
}

void UnitViewModel::drawAt(float gridX, float gridY) const
{
    drawUnit(type_, gridX, gridY, size_);
    if (maxHealth_ > 0) {
        drawUnitHealthBar(gridX, gridY, size_, static_cast<float>(health_) / static_cast<float>(maxHealth_));
    }
}

void UnitViewModel::setHealth(int health, int maxHealth) noexcept
{
    health_ = health;
    maxHealth_ = maxHealth;
}

int UnitViewModel::size() const noexcept
{
    return size_;
}

void DroneViewModel::drawAt(float gridX, float gridY) const
{
    const Vector2 ground = iso::gridToWorld(gridX, gridY);
    DrawEllipse(static_cast<int>(ground.x), static_cast<int>(ground.y), 9.0F, 5.0F, Color{0, 0, 0, 70});
    iso::drawPrism(iso::Prism{gridX - 0.18F, gridY - 0.18F, 0.36F, 0.36F, 7.0F, 7.0F},
        iso::boxColors(kDroneColor, 255));
    DrawLineEx(Vector2{ground.x - 7.0F, ground.y - 2.0F},
        Vector2{ground.x + 7.0F, ground.y - 2.0F}, 2.0F, RAYWHITE);
}

void PlayerAvatarViewModel::drawAt(float gridX, float gridY) const
{
    drawPlayerAvatar(gridX, gridY);
}

} // namespace view::gameplay
