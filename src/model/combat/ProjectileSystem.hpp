#pragma once

#include "core/EventBus.hpp"
#include "model/combat/DamageSystem.hpp"
#include "model/combat/WeaponComponents.hpp"
#include "model/ecs/Registry.hpp"

namespace model {

// Owns parabolic projectiles: creation, flight and impact resolution.
class ProjectileSystem {
public:
    ProjectileSystem(core::EventBus& bus, ecs::Registry& world, DamageSystem& damage);

    // Destroys every projectile still in flight. Call after the world is cleared.
    void reset();

    // The impact point is locked at launch. Returns the projectile entity.
    ecs::Entity launch(PlayerId owner, ecs::Entity source, ecs::Entity target, const components::Weapon& weapon,
        float startX, float startY, float impactX, float impactY);

    void update(float deltaSeconds);

private:
    core::EventBus& bus_;
    ecs::Registry& world_;
    DamageSystem& damage_;
};

} // namespace model
