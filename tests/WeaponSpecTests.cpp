#include "model/combat/WeaponType.hpp"

#include <gtest/gtest.h>

#include <initializer_list>

namespace {

constexpr std::initializer_list<model::WeaponType> kDirectKineticWeapons{
    model::WeaponType::Pistol,
    model::WeaponType::Rifle,
    model::WeaponType::MarksmanRifle,
    model::WeaponType::Shotgun,
    model::WeaponType::Cannon,
};

} // namespace

TEST(WeaponSpecTests, PistolAndRifleUseTheConfiguredRanges)
{
    // Given the pistol and the rifle specs.
    // When their ranges are read.
    // Then they match the agreed values.
    EXPECT_FLOAT_EQ(model::weaponSpec(model::WeaponType::Pistol).range, 3.0F);
    EXPECT_FLOAT_EQ(model::weaponSpec(model::WeaponType::Rifle).range, 4.0F);
}

TEST(WeaponSpecTests, ExistingArmyWeaponsAreDirectKineticSingleTarget)
{
    // Given every weapon already used by the sandbox.
    // When their behaviour is read.
    // Then they are direct, kinetic, single-target shots with damage.
    for (const model::WeaponType type : kDirectKineticWeapons) {
        const model::WeaponSpec spec = model::weaponSpec(type);
        EXPECT_EQ(spec.ballistic, model::Ballistic::Direct);
        EXPECT_EQ(spec.damageType, model::DamageType::Kinetic);
        EXPECT_FLOAT_EQ(spec.areaRadius, 0.0F);
        EXPECT_GT(spec.damage, 0);
        EXPECT_GT(spec.range, 0.0F);
        EXPECT_GT(spec.cooldown, 0.0F);
    }
}

TEST(WeaponSpecTests, MortarIsTheParabolicExplosiveWeapon)
{
    // Given the mortar spec used by the ballistics tests.
    // When its behaviour is read.
    // Then it arcs over obstacles and covers an area.
    const model::WeaponSpec spec = model::weaponSpec(model::WeaponType::Mortar);
    EXPECT_EQ(spec.ballistic, model::Ballistic::Parabolic);
    EXPECT_EQ(spec.damageType, model::DamageType::Explosive);
    EXPECT_GT(spec.areaRadius, 0.0F);
    EXPECT_GT(spec.projectileSpeed, 0.0F);
}
