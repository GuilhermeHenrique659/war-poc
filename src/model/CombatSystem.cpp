#include "model/CombatSystem.hpp"

#include "core/Events.hpp"
#include "model/components/Owner.hpp"
#include "model/components/BuildingComponents.hpp"
#include "model/components/UnitComponents.hpp"

#include <cmath>
#include <vector>

namespace model {

namespace {
constexpr float kAttackCooldown = 1.0F;
}

CombatSystem::CombatSystem(core::EventBus& bus, ecs::Registry& world, UnitSystem& units, BuildingSystem& buildings)
    : bus_(bus)
    , world_(world)
    , units_(units)
    , buildings_(buildings)
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
    const auto* owner = world_.tryGet<components::Owner>(target);
    return owner != nullptr && owner->player != player;
}

void CombatSystem::orderAttack(PlayerId player, const std::vector<ecs::Entity>& attackers, ecs::Entity target)
{
    if (!validTarget(player, target)) {
        return;
    }
    for (const ecs::Entity entity : attackers) {
        const auto* owner = world_.tryGet<components::Owner>(entity);
        if (owner == nullptr || owner->player != player || world_.tryGet<components::Combat>(entity) == nullptr
            || world_.tryGet<components::Position>(entity) == nullptr) {
            continue;
        }
        world_.emplace<components::AttackOrder>(entity, target, kAttackCooldown);
    }
}

void CombatSystem::update(float deltaSeconds)
{
    // Towers acquire the nearest enemy unit that is already inside their range.
    world_.each<components::Building>([this](ecs::Entity tower, components::Building& building) {
        if (building.type != BuildingType::Tower) {
            return;
        }
        const auto* towerOwner = world_.tryGet<components::Owner>(tower);
        const auto* towerPosition = world_.tryGet<components::Position>(tower);
        const auto* towerCombat = world_.tryGet<components::Combat>(tower);
        if (towerOwner == nullptr || towerPosition == nullptr || towerCombat == nullptr) {
            return;
        }
        ecs::Entity nearest = 0;
        float nearestDistance = towerCombat->range * towerCombat->range;
        world_.each<components::Unit>([&](ecs::Entity target, components::Unit&) {
            const auto* owner = world_.tryGet<components::Owner>(target);
            const auto* position = world_.tryGet<components::Position>(target);
            if (owner == nullptr || owner->player == towerOwner->player || position == nullptr
                || world_.tryGet<components::Unit>(target) == nullptr
                || world_.tryGet<components::Health>(target) == nullptr) {
                return;
            }
            const float dx = position->x - towerPosition->x;
            const float dy = position->y - towerPosition->y;
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

    std::vector<ecs::Entity> dead;
    world_.each<components::AttackOrder>([&](ecs::Entity entity, components::AttackOrder& order) {
        auto* health = world_.tryGet<components::Health>(order.target);
        auto* attacker = world_.tryGet<components::Combat>(entity);
        const auto* sourcePosition = world_.tryGet<components::Position>(entity);
        const auto* targetPosition = world_.tryGet<components::Position>(order.target);
        const auto* owner = world_.tryGet<components::Owner>(entity);
        if (health == nullptr || attacker == nullptr || sourcePosition == nullptr || targetPosition == nullptr
            || owner == nullptr || !validTarget(owner->player, order.target)) {
            dead.push_back(entity);
            return;
        }

        const float dx = sourcePosition->x - targetPosition->x;
        const float dy = sourcePosition->y - targetPosition->y;
        if (dx * dx + dy * dy > attacker->range * attacker->range) {
            return;
        }
        order.cooldownRemaining -= deltaSeconds;
        if (order.cooldownRemaining > 0.0F) {
            return;
        }
        health->current -= attacker->damage;
        bus_.publish(events::HealthChanged{order.target, health->current, health->max});
        order.cooldownRemaining += kAttackCooldown;
        if (health->current <= 0) {
            dead.push_back(order.target);
        }
    });

    for (const ecs::Entity entity : dead) {
        if (world_.tryGet<components::Unit>(entity) != nullptr) {
            units_.remove(entity);
        } else {
            buildings_.remove(entity);
        }
    }
}

} // namespace model
