#include "model/build/BuildingFactory.hpp"

#include "model/build/BuildingComponents.hpp"
#include "model/combat/WeaponComponents.hpp"
#include "model/economy/ProductionComponents.hpp"

#include <vector>

namespace model {

void completeBuilding(ecs::Registry& world, ecs::Entity entity, BuildingType type)
{
    if (!world.isAlive(entity) || world.has<components::Building>(entity)) {
        return;
    }
    world.emplace<components::Building>(entity, type);
    if (type == BuildingType::Tower) {
        const WeaponSpec cannon = weaponSpec(WeaponType::Cannon);
        world.emplace<components::Weapon>(entity, WeaponType::Cannon, cannon.damage, cannon.range);
    }
    switch (type) {
    case BuildingType::IronMine:
        world.emplace<components::Production>(entity, std::vector<ProductionInput>{}, ItemId::Iron, 1.0F);
        break;
    case BuildingType::CoalMine:
        world.emplace<components::Production>(entity, std::vector<ProductionInput>{}, ItemId::Coal, 1.0F);
        break;
    case BuildingType::Steelworks:
        world.emplace<components::Production>(entity,
            std::vector<ProductionInput>{{ItemId::Iron, 1.0F}, {ItemId::Coal, 1.0F}}, ItemId::Steel, 0.5F);
        break;
    case BuildingType::CopperMine:
        world.emplace<components::Production>(entity, std::vector<ProductionInput>{}, ItemId::Copper, 1.0F);
        break;
    case BuildingType::ElectronicsFactory:
        world.emplace<components::Production>(entity,
            std::vector<ProductionInput>{{ItemId::Iron, 1.0F}, {ItemId::Copper, 1.0F}}, ItemId::Electronics, 0.5F);
        break;
    case BuildingType::ToolFactory:
        world.emplace<components::Production>(entity,
            std::vector<ProductionInput>{{ItemId::Steel, 1.0F}, {ItemId::Electronics, 1.0F}}, ItemId::Tools, 0.5F);
        break;
    case BuildingType::WeaponFactory:
        world.emplace<components::Production>(entity,
            std::vector<ProductionInput>{
                {ItemId::Steel, 1.0F}, {ItemId::Electronics, 1.0F}, {ItemId::Tools, 1.0F}},
            ItemId::Weapon, 0.5F);
        break;
    case BuildingType::AmmoFactory:
        world.emplace<components::Production>(entity,
            std::vector<ProductionInput>{{ItemId::Steel, 1.0F}, {ItemId::Copper, 1.0F}}, ItemId::Ammo, 0.5F);
        break;
    case BuildingType::CommandCenter:
    case BuildingType::Wall:
    case BuildingType::House:
    case BuildingType::Generator:
    case BuildingType::Warehouse:
    case BuildingType::Barracks:
    case BuildingType::Cruzador:
    case BuildingType::Tower:
    case BuildingType::TrainingDummy:
        break;
    }
}

} // namespace model
