#include "model/units/CombatSystem.hpp"

#include "core/Events.hpp"
#include "model/FootprintGeometry.hpp"
#include "model/build/BuildingComponents.hpp"
#include "model/combat/LineOfSight.hpp"
#include "model/combat/WeaponComponents.hpp"
#include "model/components/Owner.hpp"
#include "model/economy/InventoryComponents.hpp"
#include "model/economy/InventorySystem.hpp"
#include "model/economy/Item.hpp"
#include "model/player/PlayerComponents.hpp"
#include "model/units/UnitComponents.hpp"

#include <cmath>
#include <vector>

namespace model {

namespace {

[[nodiscard]] bool tileInFootprint(GridPos tile, const components::Footprint& footprint)
{
    return tile.x >= footprint.origin.x && tile.x < footprint.origin.x + footprint.size
        && tile.y >= footprint.origin.y && tile.y < footprint.origin.y + footprint.size;
}

[[nodiscard]] GridPos tileOf(const components::Position& position)
{
    return GridPos{static_cast<int>(std::floor(position.x)), static_cast<int>(std::floor(position.y))};
}



// Damage is resolved after the AttackOrder pass so that defeats cannot invalidate the iteration.
struct PendingShot {
    ecs::Entity source;
    ecs::Entity target;
    PlayerId owner;
    components::Weapon weapon;
    float sourceX;
    float sourceY;
    float targetX;
    float targetY;
    Ballistic ballistic;
    DamageSpec damage;
};

} // namespace

CombatSystem::CombatSystem(core::EventBus& bus, ecs::Registry& world, UnitSystem& units, BuildingSystem& buildings,
    DamageSystem& damage, ProjectileSystem& projectiles)
    : bus_(bus)
    , world_(world)
    , units_(units)
    , buildings_(buildings)
    , damage_(damage)
    , projectiles_(projectiles)
    , tileSightBlocker_([this](GridPos tile) { return buildings_.isOccupied(tile); })
{
    bus_.subscribe<events::AttackOrderRequested>([this](const auto& event) {
        orderAttack(event.player, event.attackers, event.target);
    });
}

void CombatSystem::reset()
{
    world_.each<components::AttackOrder>([this](ecs::Entity entity, components::AttackOrder&) {
        world_.remove<components::AttackOrder>(entity);
    });
}

bool CombatSystem::validTarget(PlayerId player, ecs::Entity target) const
{
    if (!world_.isAlive(target) || world_.tryGet<components::Health>(target) == nullptr) {
        return false;
    }
    // A training dummy is a practice target: its own player's units may attack it too.
    const auto* building = world_.tryGet<components::Building>(target);
    if (building != nullptr && building->type == BuildingType::TrainingDummy) {
        return true;
    }
    const auto* owner = world_.tryGet<components::Owner>(target);
    return owner != nullptr && owner->player != player;
}

bool CombatSystem::canFire(ecs::Entity entity) const
{
    if (world_.tryGet<components::Unit>(entity) == nullptr) {
        return true;
    }
    const auto* inventory = world_.tryGet<components::Inventory>(entity);
    if (inventory == nullptr) {
        return false;
    }
    return InventorySystem::amount(*inventory, ItemId::Weapon) >= kUnitWeaponCount
        && InventorySystem::amount(*inventory, ItemId::Ammo) > 0;
}

bool CombatSystem::hasLineOfSightTo(ecs::Entity source, ecs::Entity target) const
{
    const auto* sourcePosition = world_.tryGet<components::Position>(source);
    const auto* targetPosition = world_.tryGet<components::Position>(target);
    if (sourcePosition == nullptr || targetPosition == nullptr) {
        return false;
    }
    const auto* sourceFootprint = world_.tryGet<components::Footprint>(source);
    const auto* targetFootprint = world_.tryGet<components::Footprint>(target);

    return combat::hasLineOfSight(tileOf(*sourcePosition), tileOf(*targetPosition),
        [this, sourceFootprint, targetFootprint](GridPos tile) {
            // A shooter or target standing inside a building never blocks its own shot.
            if (sourceFootprint != nullptr && tileInFootprint(tile, *sourceFootprint)) {
                return false;
            }
            if (targetFootprint != nullptr && tileInFootprint(tile, *targetFootprint)) {
                return false;
            }
            return tileSightBlocker_(tile);
        });
}

GridPos CombatSystem::targetTile(ecs::Entity target) const
{
    if (const auto* footprint = world_.tryGet<components::Footprint>(target); footprint != nullptr) {
        return footprintCenter(*footprint);
    }
    const auto* position = world_.tryGet<components::Position>(target);
    return position == nullptr ? GridPos{0, 0} : tileOf(*position);
}

void CombatSystem::consumeAmmo(ecs::Entity entity) const
{
    if (world_.tryGet<components::Unit>(entity) == nullptr) {
        return;
    }
    if (auto* inventory = world_.tryGet<components::Inventory>(entity); inventory != nullptr) {
        (void)InventorySystem::remove(*inventory, ItemId::Ammo, 1);
    }
}

void CombatSystem::orderAttack(PlayerId player, const std::vector<ecs::Entity>& attackers, ecs::Entity target)
{
    if (!validTarget(player, target)) {
        return;
    }
    for (const ecs::Entity entity : attackers) {
        const auto* owner = world_.tryGet<components::Owner>(entity);
        const auto* weapon = world_.tryGet<components::Weapon>(entity);
        const auto* position = world_.tryGet<components::Position>(entity);
        if (owner == nullptr || owner->player != player || weapon == nullptr || position == nullptr) {
            continue;
        }
        world_.emplace<components::AttackOrder>(entity, target, weaponSpec(weapon->type).cooldown);

        const auto* targetPosition = world_.tryGet<components::Position>(target);
        if (targetPosition == nullptr) {
            continue;
        }
        const float dx = position->x - targetPosition->x;
        const float dy = position->y - targetPosition->y;
        if (dx * dx + dy * dy <= weapon->range * weapon->range) {
            // Already in range: stop walking so the unit fires instead of leaving the target.
            const auto* mover = world_.tryGet<components::Mover>(entity);
            if (mover != nullptr && !mover->path.empty()) {
                units_.stop(entity);
            }
            continue;
        }
        (void)units_.moveWithinRange(entity, targetTile(target), weapon->range);
    }
}

void CombatSystem::update(float deltaSeconds)
{
    // Towers acquire the nearest enemy unit or player avatar already inside their range.
    world_.each<components::Building>([this](ecs::Entity tower, components::Building& building) {
        if (building.type != BuildingType::Tower) {
            return;
        }
        const auto* towerOwner = world_.tryGet<components::Owner>(tower);
        const auto* towerPosition = world_.tryGet<components::Position>(tower);
        const auto* towerWeapon = world_.tryGet<components::Weapon>(tower);
        if (towerOwner == nullptr || towerPosition == nullptr || towerWeapon == nullptr) {
            return;
        }
        ecs::Entity nearest = 0;
        float nearestDistance = towerWeapon->range * towerWeapon->range;
        world_.each<components::Position>([&](ecs::Entity target, const components::Position& position) {
            const auto* owner = world_.tryGet<components::Owner>(target);
            const auto* health = world_.tryGet<components::Health>(target);
            const bool isCombatUnit = world_.tryGet<components::Unit>(target) != nullptr
                || world_.tryGet<components::PlayerProfile>(target) != nullptr;
            if (owner == nullptr || owner->player == towerOwner->player || health == nullptr
                || health->current <= 0 || !isCombatUnit) {
                return;
            }
            const float dx = position.x - towerPosition->x;
            const float dy = position.y - towerPosition->y;
            const float distance = dx * dx + dy * dy;
            if (distance <= nearestDistance) {
                nearestDistance = distance;
                nearest = target;
            }
        });
        if (nearest == 0) {
            world_.remove<components::AttackOrder>(tower);
            return;
        }
        auto* order = world_.tryGet<components::AttackOrder>(tower);
        if (order == nullptr || order->target != nearest) {
            world_.emplace<components::AttackOrder>(tower, nearest, 0.0F);
        }
    });

    std::vector<ecs::Entity> invalidOrders;
    std::vector<PendingShot> shots;
    world_.each<components::AttackOrder>([&](ecs::Entity entity, components::AttackOrder& order) {
        auto* health = world_.tryGet<components::Health>(order.target);
        auto* weapon = world_.tryGet<components::Weapon>(entity);
        const auto* sourcePosition = world_.tryGet<components::Position>(entity);
        const auto* targetPosition = world_.tryGet<components::Position>(order.target);
        const auto* owner = world_.tryGet<components::Owner>(entity);
        if (health == nullptr || weapon == nullptr || sourcePosition == nullptr || targetPosition == nullptr
            || owner == nullptr || !validTarget(owner->player, order.target)) {
            invalidOrders.push_back(entity);
            return;
        }
        if (health->current <= 0) {
            invalidOrders.push_back(entity);
            return;
        }

        const float dx = sourcePosition->x - targetPosition->x;
        const float dy = sourcePosition->y - targetPosition->y;
        if (dx * dx + dy * dy > weapon->range * weapon->range) {
            // A fresh attack order (or a finished resupply trip) walks until the target is in range.
            const auto* mover = world_.tryGet<components::Mover>(entity);
            if (mover != nullptr && mover->path.empty()) {
                (void)units_.moveWithinRange(entity, targetTile(order.target), weapon->range);
            }
            return;
        }
        order.cooldownRemaining -= deltaSeconds;
        if (order.cooldownRemaining > 0.0F) {
            return;
        }
        if (!canFire(entity)) {
            return;
        }

        const WeaponSpec spec = weaponSpec(weapon->type);
        if (spec.ballistic == Ballistic::Direct && !hasLineOfSightTo(entity, order.target)) {
            // Hold fire without spending ammo or moving the cooldown.
            return;
        }

        consumeAmmo(entity);
        order.cooldownRemaining += spec.cooldown;
        shots.push_back(PendingShot{entity, order.target, owner->player, *weapon, sourcePosition->x,
            sourcePosition->y, targetPosition->x, targetPosition->y, spec.ballistic,
            DamageSpec{spec.damageType, weapon->damage, spec.areaRadius}});
    });

    for (const ecs::Entity entity : invalidOrders) {
        if (world_.isAlive(entity)) {
            world_.remove<components::AttackOrder>(entity);
        }
    }

    // Resolving damage here keeps any defeat from invalidating the AttackOrder traversal above.
    for (const PendingShot& shot : shots) {
        if (shot.ballistic == Ballistic::Parabolic) {
            (void)projectiles_.launch(
                shot.owner, shot.source, shot.target, shot.weapon, shot.sourceX, shot.sourceY, shot.targetX,
                shot.targetY);
            continue;
        }
        if (shot.damage.type == DamageType::Explosive && shot.damage.areaRadius > 0.0F) {
            (void)damage_.applyArea(shot.targetX, shot.targetY, shot.damage);
            continue;
        }
        (void)damage_.apply(shot.target, shot.damage);
    }
}

} // namespace model
