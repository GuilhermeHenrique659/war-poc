#pragma once

#include "model/build/BuildingType.hpp"
#include "model/units/UnitType.hpp"
#include "view/build/BuildingSpriteViewModel.hpp"
#include "view/build/FactoryViewModel.hpp"
#include "view/units/DroneViewModel.hpp"
#include "view/world/WorldObjectViewModel.hpp"

namespace view::gameplay {

class MineResourceViewModel;

class BuildingViewModel final : public WorldObjectViewModel {
public:
    BuildingViewModel(model::BuildingType type, int size, const MineResourceViewModel& mineResourceViewModel,
        const BuildingSpriteViewModel& commandCenterViewModel, const BuildingSpriteViewModel& warehouseViewModel,
        const BuildingSpriteViewModel& steelworksViewModel, const FactoryViewModel& factoryViewModel,
        const BuildingSpriteViewModel& barracksViewModel);

    void drawAt(float gridX, float gridY) const override;
    void drawAt(float gridX, float gridY, unsigned char alpha) const;

private:
    model::BuildingType type_;
    int size_;
    const MineResourceViewModel& mineResourceViewModel_;
    const BuildingSpriteViewModel& commandCenterViewModel_;
    const BuildingSpriteViewModel& warehouseViewModel_;
    const BuildingSpriteViewModel& steelworksViewModel_;
    const FactoryViewModel& factoryViewModel_;
    const BuildingSpriteViewModel& barracksViewModel_;
};

class UnitViewModel final : public WorldObjectViewModel {
public:
    UnitViewModel(model::UnitType type, int size, int health, int maxHealth);

    void drawAt(float gridX, float gridY) const override;
    void setHealth(int health, int maxHealth) noexcept;

private:
    model::UnitType type_;
    int size_;
    int health_;
    int maxHealth_;
};

class PlayerAvatarViewModel final : public WorldObjectViewModel {
public:
    void drawAt(float gridX, float gridY) const override;
};

} // namespace view::gameplay
