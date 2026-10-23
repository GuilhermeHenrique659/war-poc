#include "view/world/WorldEntityViewModels.hpp"

#include "view/build/BuildingRenderer.hpp"
#include "view/build/MineResourceViewModel.hpp"
#include "view/build/WarehouseViewModel.hpp"
#include "view/player/PlayerRenderer.hpp"
#include "view/units/UnitRenderer.hpp"

namespace view::gameplay {

BuildingViewModel::BuildingViewModel(
    model::BuildingType type, int size, const MineResourceViewModel& mineResourceViewModel,
    const CommandCenterViewModel& commandCenterViewModel, const WarehouseViewModel& warehouseViewModel,
    const SteelworksViewModel& steelworksViewModel)
    : type_(type)
    , size_(size)
    , mineResourceViewModel_(mineResourceViewModel)
    , commandCenterViewModel_(commandCenterViewModel)
    , warehouseViewModel_(warehouseViewModel)
    , steelworksViewModel_(steelworksViewModel)
{
}

void BuildingViewModel::drawAt(float gridX, float gridY) const
{
    if (type_ == model::BuildingType::CommandCenter) {
        commandCenterViewModel_.drawAt(gridX, gridY, size_);
        return;
    }
    if (MineResourceViewModel::handles(type_)) {
        mineResourceViewModel_.drawAt(type_, gridX, gridY, size_);
        return;
    }
    if (type_ == model::BuildingType::Warehouse) {
        warehouseViewModel_.drawAt(gridX, gridY, size_);
        return;
    }
    if (SteelworksViewModel::handles(type_)) {
        steelworksViewModel_.drawAt(gridX, gridY, size_);
        return;
    }
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

void PlayerAvatarViewModel::drawAt(float gridX, float gridY) const
{
    drawPlayerAvatar(gridX, gridY);
}

} // namespace view::gameplay
