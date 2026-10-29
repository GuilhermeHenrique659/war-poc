#include "view/world/WorldEntityViewModels.hpp"

#include "view/build/BuildingRenderer.hpp"
#include "view/build/MineResourceViewModel.hpp"
#include "view/player/PlayerRenderer.hpp"
#include "view/units/UnitRenderer.hpp"

namespace view::gameplay {

BuildingViewModel::BuildingViewModel(
    model::BuildingType type, int size, const MineResourceViewModel& mineResourceViewModel,
    const BuildingSpriteViewModel& commandCenterViewModel, const BuildingSpriteViewModel& warehouseViewModel,
    const BuildingSpriteViewModel& steelworksViewModel, const FactoryViewModel& factoryViewModel,
    const BuildingSpriteViewModel& barracksViewModel)
    : type_(type)
    , size_(size)
    , mineResourceViewModel_(mineResourceViewModel)
    , commandCenterViewModel_(commandCenterViewModel)
    , warehouseViewModel_(warehouseViewModel)
    , steelworksViewModel_(steelworksViewModel)
    , factoryViewModel_(factoryViewModel)
    , barracksViewModel_(barracksViewModel)
{
}

void BuildingViewModel::drawAt(float gridX, float gridY) const
{
    drawAt(gridX, gridY, 255);
}

void BuildingViewModel::drawAt(float gridX, float gridY, unsigned char alpha) const
{
    if (type_ == model::BuildingType::CommandCenter) {
        commandCenterViewModel_.drawAt(gridX, gridY, size_, alpha);
        return;
    }
    if (MineResourceViewModel::handles(type_)) {
        mineResourceViewModel_.drawAt(type_, gridX, gridY, size_, alpha);
        return;
    }
    if (type_ == model::BuildingType::Warehouse) {
        warehouseViewModel_.drawAt(gridX, gridY, size_, alpha);
        return;
    }
    if (type_ == model::BuildingType::Steelworks) {
        steelworksViewModel_.drawAt(gridX, gridY, size_, alpha);
        return;
    }
    if (FactoryViewModel::handles(type_)) {
        factoryViewModel_.drawAt(type_, gridX, gridY, size_, alpha);
        return;
    }
    if (type_ == model::BuildingType::Barracks) {
        barracksViewModel_.drawAt(gridX, gridY, size_, alpha);
        return;
    }
    drawBuilding(type_, model::GridPos{static_cast<int>(gridX), static_cast<int>(gridY)}, size_, alpha);
}

UnitViewModel::UnitViewModel(model::UnitType type, int size, int health, int maxHealth)
    : type_(type)
    , size_(size)
    , health_(health)
    , maxHealth_(maxHealth)
{
    if (type == model::UnitType::Soldier) {
        soldierSprite_.emplace();
    }
}

void UnitViewModel::drawAt(float gridX, float gridY) const
{
    drawUnit(type_, gridX, gridY, size_, facing_, soldierSprite_ ? &*soldierSprite_ : nullptr);
    if (maxHealth_ > 0) {
        drawUnitHealthBar(gridX, gridY, size_, static_cast<float>(health_) / static_cast<float>(maxHealth_));
    }
}

void UnitViewModel::setHealth(int health, int maxHealth) noexcept
{
    health_ = health;
    maxHealth_ = maxHealth;
}

void UnitViewModel::setFacing(UnitFacing facing) noexcept
{
    facing_ = facing;
}

UnitFacing UnitViewModel::facing() const noexcept
{
    return facing_;
}

void PlayerAvatarViewModel::drawAt(float gridX, float gridY) const
{
    drawPlayerAvatar(gridX, gridY);
}

void PlayerAvatarViewModel::setFacing(UnitFacing facing) noexcept
{
    facing_ = facing;
}

UnitFacing PlayerAvatarViewModel::facing() const noexcept
{
    return facing_;
}

} // namespace view::gameplay
