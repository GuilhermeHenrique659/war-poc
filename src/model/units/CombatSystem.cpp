#include "model/units/CombatSystem.hpp"

#include "core/Events.hpp"
#include "model/components/Owner.hpp"
#include "model/build/BuildingComponents.hpp"
#include "model/player/PlayerComponents.hpp"
#include "model/units/UnitComponents.hpp"

#include <cmath>
#include <unordered_set>
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
    // Towers acquire the nearest enemy unit or player avatar already inside their range.
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

    std::vector<ecs::Entity> defeated;
    std::vector<ecs::Entity> invalidOrders;
    world_.each<components::AttackOrder>([&](ecs::Entity entity, components::AttackOrder& order) {
        auto* health = world_.tryGet<components::Health>(order.target);
        auto* attacker = world_.tryGet<components::Combat>(entity);
        const auto* sourcePosition = world_.tryGet<components::Position>(entity);
        const auto* targetPosition = world_.tryGet<components::Position>(order.target);
        const auto* owner = world_.tryGet<components::Owner>(entity);
        if (health == nullptr || attacker == nullptr || sourcePosition == nullptr || targetPosition == nullptr
            || owner == nullptr || !validTarget(owner->player, order.target)) {
            invalidOrders.push_back(entity);
            return;
        }
        if (health->current <= 0) {
            defeated.push_back(order.target);
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
            defeated.push_back(order.target);
        }
    });

    for (const ecs::Entity entity : invalidOrders) {
        if (world_.isAlive(entity)) {
            world_.remove<components::AttackOrder>(entity);
        }
    }
    std::unordered_set<ecs::Entity> handledDefeats;
    for (const ecs::Entity entity : defeated) {
        if (!handledDefeats.insert(entity).second || !world_.isAlive(entity)) {
            continue;
        }
        if (world_.tryGet<components::PlayerProfile>(entity) != nullptr) {
            bus_.publish(events::PlayerRespawnRequested{entity});
            continue;
        }
        if (world_.tryGet<components::Unit>(entity) != nullptr) {
            units_.remove(entity);
        } else {
            buildings_.remove(entity);
        }
    }
}

} // namespace model
