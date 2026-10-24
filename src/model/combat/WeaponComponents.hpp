#pragma once

#include "model/combat/WeaponType.hpp"
#include "model/components/Owner.hpp"
#include "model/ecs/Entity.hpp"

namespace model::components {

// Instance values default to the weapon spec when the entity is created. Genealogy may combine
// them, but the weapon type keeps defining ballistics, damage type and area.
struct Weapon {
    WeaponType type = WeaponType::Pistol;
    int damage = 0;
    float range = 0.0F;
};

// A parabolic shot in flight. It deliberately has no Position/Health component: projectile
// entities must not become tower targets or be caught by their own explosion.
struct Projectile {
    ecs::Entity source = 0;
    ecs::Entity target = 0;
    PlayerId owner = kNoPlayer;
    WeaponType type = WeaponType::Pistol;
    DamageType damageType = DamageType::Kinetic;
    int damage = 0;
    float areaRadius = 0.0F;
    float startX = 0.0F;
    float startY = 0.0F;
    float x = 0.0F;
    float y = 0.0F;
    float impactX = 0.0F; // Locked at launch.
    float impactY = 0.0F;
    float elapsed = 0.0F;
    float flightTime = 0.0F;
};

} // namespace model::components
