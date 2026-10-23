#include "model/units/UnitSupplySystem.hpp"

#include "model/build/BuildingComponents.hpp"
#include "model/components/Owner.hpp"
#include "model/economy/InventorySystem.hpp"
#include "model/economy/Item.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace model {

namespace {

constexpr float kArrivalRadius = 2.5F;

[[nodiscard]] bool hasAny(const components::Inventory& inventory, ItemId item)
{
    return InventorySystem::amount(inventory, item) > 0;
}

[[nodiscard]] bool isFullySupplied(const components::Inventory& inventory)
{
    return InventorySystem::amount(inventory, ItemId::Weapon) >= kUnitWeaponCount
        && InventorySystem::amount(inventory, ItemId::Ammo) >= kUnitAmmoCapacity;
}

[[nodiscard]] float distanceSquared(
    const components::Position& position, const components::Footprint& footprint)
{
    const float centerX = static_cast<float>(footprint.origin.x) + static_cast<float>(footprint.size) / 2.0F;
    const float centerY = static_cast<float>(footprint.origin.y) + static_cast<float>(footprint.size) / 2.0F;
    const float dx = position.x - centerX;
    const float dy = position.y - centerY;
    return dx * dx + dy * dy;
}

} // namespace

UnitSupplySystem::UnitSupplySystem(ecs::Registry& world, UnitSystem& units)
    : world_(world)
    , units_(units)
{
}

void UnitSupplySystem::update()
{
    std::vector<ecs::Entity> units;
    world_.each<components::Unit>(
        [&units](ecs::Entity entity, const components::Unit&) { units.push_back(entity); });
    for (const ecs::Entity entity : units) {
        if (world_.isAlive(entity)) {
            updateUnit(entity);
        }
    }
}

void UnitSupplySystem::updateUnit(ecs::Entity entity)
{
    auto* inventory = world_.tryGet<components::Inventory>(entity);
    auto* state = world_.tryGet<components::UnitSupplyState>(entity);
    if (inventory == nullptr || state == nullptr) {
        return;
    }

    const bool needsWeapon = InventorySystem::amount(*inventory, ItemId::Weapon) < kUnitWeaponCount;
    const bool needsAmmo = InventorySystem::amount(*inventory, ItemId::Ammo) == 0;
    if (!needsWeapon && !needsAmmo) {
        state->activity = UnitActivity::Idle;
        state->supplyTarget.reset();
        return;
    }

    const auto target = findSupplyTarget(entity, needsWeapon, needsAmmo);
    if (!target) {
        state->activity = UnitActivity::WaitingForSupplies;
        state->supplyTarget.reset();
        stopIfMoving(entity);
        return;
    }

    const bool alreadySeeking
        = state->activity == UnitActivity::SeekingSupplies && state->supplyTarget == target;
    state->supplyTarget = target;

    if (isNearSupply(entity, *target)) {
        refill(*inventory, *target);
        state->activity = isFullySupplied(*inventory) ? UnitActivity::Idle
                                                      : UnitActivity::WaitingForSupplies;
        if (state->activity == UnitActivity::Idle) {
            state->supplyTarget.reset();
        }
        return;
    }

    if (alreadySeeking) {
        const auto* mover = world_.tryGet<components::Mover>(entity);
        if (mover != nullptr && !mover->path.empty()) {
            return;
        }
        // The path ended away from the target (blocked or unreachable): wait here.
        state->activity = UnitActivity::WaitingForSupplies;
        stopIfMoving(entity);
        return;
    }

    const auto* footprint = world_.tryGet<components::Footprint>(*target);
    if (footprint != nullptr && units_.moveToAdjacent(entity,
            GridPos{footprint->origin.x + footprint->size / 2, footprint->origin.y + footprint->size / 2})) {
        state->activity = UnitActivity::SeekingSupplies;
        return;
    }
    state->activity = UnitActivity::WaitingForSupplies;
}

std::optional<ecs::Entity> UnitSupplySystem::findSupplyTarget(
    ecs::Entity unit, bool needsWeapon, bool needsAmmo) const
{
    const auto* owner = world_.tryGet<components::Owner>(unit);
    const auto* position = world_.tryGet<components::Position>(unit);
    if (owner == nullptr || position == nullptr) {
        return std::nullopt;
    }

    std::optional<ecs::Entity> best;
    float bestDistance = 0.0F;
    bool bestIsWarehouse = false;
    world_.each<components::Building>([&](ecs::Entity entity, const components::Building& building) {
        if (building.type != BuildingType::Warehouse && building.type != BuildingType::CommandCenter) {
            return;
        }
        const auto* buildingOwner = world_.tryGet<components::Owner>(entity);
        const auto* footprint = world_.tryGet<components::Footprint>(entity);
        const auto* inventory = world_.tryGet<components::Inventory>(entity);
        if (buildingOwner == nullptr || buildingOwner->player != owner->player || footprint == nullptr
            || inventory == nullptr) {
            return;
        }
        const bool provides = (needsWeapon && hasAny(*inventory, ItemId::Weapon))
            || (needsAmmo && hasAny(*inventory, ItemId::Ammo));
        if (!provides) {
            return;
        }

        const bool warehouse = building.type == BuildingType::Warehouse;
        const float distance = distanceSquared(*position, *footprint);
        const bool better = !best || (warehouse && !bestIsWarehouse)
            || (warehouse == bestIsWarehouse && distance < bestDistance);
        if (better) {
            best = entity;
            bestDistance = distance;
            bestIsWarehouse = warehouse;
        }
    });
    return best;
}

bool UnitSupplySystem::isNearSupply(ecs::Entity unit, ecs::Entity supply) const
{
    const auto* position = world_.tryGet<components::Position>(unit);
    const auto* footprint = world_.tryGet<components::Footprint>(supply);
    if (position == nullptr || footprint == nullptr) {
        return false;
    }
    const float left = static_cast<float>(footprint->origin.x);
    const float top = static_cast<float>(footprint->origin.y);
    const float right = left + static_cast<float>(footprint->size);
    const float bottom = top + static_cast<float>(footprint->size);
    const float nearestX = std::clamp(position->x, left, right);
    const float nearestY = std::clamp(position->y, top, bottom);
    return std::hypot(position->x - nearestX, position->y - nearestY) <= kArrivalRadius;
}

void UnitSupplySystem::refill(components::Inventory& inventory, ecs::Entity supply) const
{
    auto* source = world_.tryGet<components::Inventory>(supply);
    if (source == nullptr) {
        return;
    }

    const int missingWeapons = kUnitWeaponCount - InventorySystem::amount(inventory, ItemId::Weapon);
    if (missingWeapons > 0) {
        (void)InventorySystem::transfer(*source, inventory, ItemId::Weapon, missingWeapons);
    }
    const int missingAmmo = kUnitAmmoCapacity - InventorySystem::amount(inventory, ItemId::Ammo);
    if (missingAmmo > 0) {
        (void)InventorySystem::transfer(*source, inventory, ItemId::Ammo, missingAmmo);
    }
}

void UnitSupplySystem::stopIfMoving(ecs::Entity entity) const
{
    const auto* mover = world_.tryGet<components::Mover>(entity);
    if (mover != nullptr && !mover->path.empty()) {
        units_.stop(entity);
    }
}

} // namespace model
