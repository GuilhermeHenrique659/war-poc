#pragma once

#include "model/build/BuildingType.hpp"
#include "model/units/UnitType.hpp"
#include "view/world/WorldObjectViewModel.hpp"

namespace view::gameplay {

class MineResourceViewModel;

class BuildingViewModel final : public WorldObjectViewModel {
public:
    BuildingViewModel(model::BuildingType type, int size, const MineResourceViewModel& mineResourceViewModel);

    void drawAt(float gridX, float gridY) const override;

private:
    model::BuildingType type_;
    int size_;
    const MineResourceViewModel& mineResourceViewModel_;
};

class UnitViewModel final : public WorldObjectViewModel {
public:
    UnitViewModel(model::UnitType type, int size, int health, int maxHealth);

    void drawAt(float gridX, float gridY) const override;
    void setHealth(int health, int maxHealth) noexcept;
    [[nodiscard]] int size() const noexcept;

private:
    model::UnitType type_;
    int size_;
    int health_;
    int maxHealth_;
};

class DroneViewModel final : public WorldObjectViewModel {
public:
    void drawAt(float gridX, float gridY) const override;
};

class PlayerAvatarViewModel final : public WorldObjectViewModel {
public:
    void drawAt(float gridX, float gridY) const override;
};

} // namespace view::gameplay
