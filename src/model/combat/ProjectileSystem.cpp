#include "model/combat/ProjectileSystem.hpp"

#include "core/Events.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace model {

namespace {

constexpr float kMinimumFlightTime = 0.2F;
constexpr float kMinimumProjectileSpeed = 0.1F;

[[nodiscard]] float interpolate(float start, float end, float progress)
{
    return start + (end - start) * progress;
}

} // namespace

ProjectileSystem::ProjectileSystem(core::EventBus& bus, ecs::Registry& world, DamageSystem& damage)
    : bus_(bus)
    , world_(world)
    , damage_(damage)
{
}

void ProjectileSystem::reset()
{
    std::vector<ecs::Entity> projectiles;
    world_.each<components::Projectile>(
        [&](ecs::Entity entity, const components::Projectile&) { projectiles.push_back(entity); });
    for (const ecs::Entity entity : projectiles) {
        world_.destroy(entity);
    }
}

ecs::Entity ProjectileSystem::launch(PlayerId owner, ecs::Entity source, ecs::Entity target,
    const components::Weapon& weapon, float startX, float startY, float impactX, float impactY)
{
    const WeaponSpec spec = weaponSpec(weapon.type);
    const float distance = static_cast<float>(std::hypot(impactX - startX, impactY - startY));
    const float speed = std::max(spec.projectileSpeed, kMinimumProjectileSpeed);

    const ecs::Entity entity = world_.create();
    auto& projectile = world_.emplace<components::Projectile>(entity);
    projectile.source = source;
    projectile.target = target;
    projectile.owner = owner;
    projectile.type = weapon.type;
    projectile.damageType = spec.damageType;
    projectile.damage = weapon.damage;
    projectile.areaRadius = spec.areaRadius;
    projectile.startX = startX;
    projectile.startY = startY;
    projectile.x = startX;
    projectile.y = startY;
    projectile.impactX = impactX;
    projectile.impactY = impactY;
    projectile.flightTime = std::max(distance / speed, kMinimumFlightTime);

    bus_.publish(events::ProjectileSpawned{
        entity, target, owner, weapon.type, startX, startY, impactX, impactY, projectile.flightTime});
    return entity;
}

void ProjectileSystem::update(float deltaSeconds)
{
    std::vector<ecs::Entity> projectiles;
    world_.each<components::Projectile>(
        [&](ecs::Entity entity, const components::Projectile&) { projectiles.push_back(entity); });

    std::vector<ecs::Entity> impacted;
    for (const ecs::Entity entity : projectiles) {
        auto* projectile = world_.tryGet<components::Projectile>(entity);
        if (projectile == nullptr) {
            continue;
        }
        projectile->elapsed += deltaSeconds;
        const float progress = std::min(projectile->elapsed / projectile->flightTime, 1.0F);
        projectile->x = interpolate(projectile->startX, projectile->impactX, progress);
        projectile->y = interpolate(projectile->startY, projectile->impactY, progress);

        bus_.publish(events::ProjectileMoved{entity, projectile->x, projectile->y, progress});
        if (progress >= 1.0F) {
            impacted.push_back(entity);
        }
    }

    for (const ecs::Entity entity : impacted) {
        const auto* projectile = world_.tryGet<components::Projectile>(entity);
        if (projectile == nullptr) {
            continue;
        }
        const DamageSpec damage{projectile->damageType, projectile->damage, projectile->areaRadius};
        if (damage.areaRadius > 0.0F) {
            (void)damage_.applyArea(projectile->impactX, projectile->impactY, damage);
        } else if (world_.isAlive(projectile->target)) {
            (void)damage_.apply(projectile->target, damage);
        }
        bus_.publish(events::ProjectileImpacted{entity, projectile->impactX, projectile->impactY});
        world_.destroy(entity);
    }
}

} // namespace model
