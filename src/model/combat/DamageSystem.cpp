#include "model/combat/DamageSystem.hpp"

#include "core/Events.hpp"
#include "model/FootprintGeometry.hpp"
#include "model/build/BuildingComponents.hpp"
#include "model/build/ConstructionComponents.hpp"
#include "model/player/PlayerComponents.hpp"
#include "model/units/UnitComponents.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_set>

namespace model {

DamageSystem::DamageSystem(core::EventBus& bus, ecs::Registry& world, UnitSystem& units, BuildingSystem& buildings)
    : bus_(bus)
    , world_(world)
    , units_(units)
    , buildings_(buildings)
{
}

bool DamageSystem::apply(ecs::Entity target, const DamageSpec& damage)
{
    std::vector<ecs::Entity> defeated;
    damageOne(target, damage.amount, defeated);
    resolveDefeats(defeated);
    return !defeated.empty();
}

std::size_t DamageSystem::applyArea(float x, float y, const DamageSpec& damage)
{
    if (damage.amount <= 0) {
        return 0;
    }

    std::vector<ecs::Entity> targets;
    world_.each<components::Health>([&](ecs::Entity entity, const components::Health& health) {
        if (health.current <= 0 || distanceTo(entity, x, y) > damage.areaRadius) {
            return;
        }
        targets.push_back(entity);
    });

    std::vector<ecs::Entity> defeated;
    for (const ecs::Entity entity : targets) {
        damageOne(entity, damage.amount, defeated);
    }
    resolveDefeats(defeated);
    return targets.size();
}

void DamageSystem::damageOne(ecs::Entity entity, int amount, std::vector<ecs::Entity>& defeated)
{
    if (amount <= 0) {
        return;
    }
    auto* health = world_.tryGet<components::Health>(entity);
    if (health == nullptr || health->current <= 0) {
        return;
    }

    health->current = std::max(0, health->current - amount);
    bus_.publish(events::HealthChanged{entity, health->current, health->max});
    if (health->current <= 0) {
        defeated.push_back(entity);
    }
}

void DamageSystem::resolveDefeats(const std::vector<ecs::Entity>& defeated)
{
    std::unordered_set<ecs::Entity> handled;
    for (const ecs::Entity entity : defeated) {
        if (!handled.insert(entity).second || !world_.isAlive(entity)) {
            continue;
        }
        if (world_.tryGet<components::PlayerProfile>(entity) != nullptr) {
            bus_.publish(events::PlayerRespawnRequested{entity});
            continue;
        }
        if (world_.tryGet<components::Unit>(entity) != nullptr) {
            units_.remove(entity);
            continue;
        }
        if (world_.tryGet<components::Building>(entity) != nullptr
            || world_.tryGet<components::ConstructionSite>(entity) != nullptr) {
            buildings_.remove(entity);
        }
    }
}

float DamageSystem::distanceTo(ecs::Entity entity, float x, float y) const
{
    if (const auto* footprint = world_.tryGet<components::Footprint>(entity)) {
        return distanceToFootprint(x, y, footprintBounds(*footprint));
    }
    if (const auto* position = world_.tryGet<components::Position>(entity)) {
        return static_cast<float>(std::hypot(position->x - x, position->y - y));
    }
    return std::numeric_limits<float>::infinity();
}

} // namespace model
