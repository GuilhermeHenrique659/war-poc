#pragma once

#include "core/EventBus.hpp"
#include "model/build/BuildingSystem.hpp"
#include "model/combat/WeaponType.hpp"
#include "model/ecs/Registry.hpp"
#include "model/units/UnitSystem.hpp"

#include <cstddef>
#include <vector>

namespace model {

struct DamageSpec {
    DamageType type = DamageType::Kinetic;
    int amount = 0;
    float areaRadius = 0.0F; // Tiles. Zero means a single target.
};

// Applies damage and resolves defeats (avatar respawn, unit/building removal). Callers decide
// between a single target and an area based on the weapon's damage type.
class DamageSystem {
public:
    DamageSystem(core::EventBus& bus, ecs::Registry& world, UnitSystem& units, BuildingSystem& buildings);

    // Damages only the target. Returns true when the target was defeated.
    bool apply(ecs::Entity target, const DamageSpec& damage);
    // Damages every entity with health inside the radius, friendly fire included.
    // Returns how many entities were damaged.
    std::size_t applyArea(float x, float y, const DamageSpec& damage);

private:
    void damageOne(ecs::Entity entity, int amount, std::vector<ecs::Entity>& defeated);
    void resolveDefeats(const std::vector<ecs::Entity>& defeated);
    [[nodiscard]] float distanceTo(ecs::Entity entity, float x, float y) const;

    core::EventBus& bus_;
    ecs::Registry& world_;
    UnitSystem& units_;
    BuildingSystem& buildings_;
};

} // namespace model
