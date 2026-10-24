#pragma once

#include <string_view>

namespace model {

// How the shot reaches the target. Direct shots travel in a straight line and need a clear line
// of sight; parabolic shots arc over obstacles and resolve on impact.
enum class Ballistic { Direct, Parabolic };

// What the damage does once it lands. Kinetic hits the target only; explosive covers an area.
enum class DamageType { Kinetic, Explosive };

enum class WeaponType { Pistol, Rifle, MarksmanRifle, Shotgun, Cannon, Mortar };

// Base values of a weapon. Entities copy damage and range into their Weapon component so that
// genealogy can combine them, but the behaviour (ballistics, area, cooldown) always comes from here.
struct WeaponSpec {
    std::string_view label;
    Ballistic ballistic;
    DamageType damageType;
    int damage;
    float range; // Tiles.
    float cooldown; // Seconds between shots.
    float areaRadius; // Tiles. Zero means a single target.
    float projectileSpeed; // Tiles per second. Only used by parabolic weapons.
};

[[nodiscard]] constexpr WeaponSpec weaponSpec(WeaponType type)
{
    switch (type) {
    case WeaponType::Pistol:
        return WeaponSpec{"Pistola", Ballistic::Direct, DamageType::Kinetic, 1, 3.0F, 1.0F, 0.0F, 0.0F};
    case WeaponType::Rifle:
        return WeaponSpec{"Fuzil", Ballistic::Direct, DamageType::Kinetic, 10, 4.0F, 1.0F, 0.0F, 0.0F};
    case WeaponType::MarksmanRifle:
        return WeaponSpec{"Rifle de Precisao", Ballistic::Direct, DamageType::Kinetic, 8, 5.0F, 1.0F, 0.0F, 0.0F};
    case WeaponType::Shotgun:
        return WeaponSpec{"Espingarda", Ballistic::Direct, DamageType::Kinetic, 14, 1.0F, 1.0F, 0.0F, 0.0F};
    case WeaponType::Cannon:
        return WeaponSpec{"Canhao", Ballistic::Direct, DamageType::Kinetic, 20, 6.0F, 1.0F, 0.0F, 0.0F};
    case WeaponType::Mortar:
        return WeaponSpec{"Morteiro", Ballistic::Parabolic, DamageType::Explosive, 15, 8.0F, 1.0F, 2.0F, 3.0F};
    }
    return WeaponSpec{"?", Ballistic::Direct, DamageType::Kinetic, 0, 0.0F, 1.0F, 0.0F, 0.0F};
}

} // namespace model
