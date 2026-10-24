#pragma once

#include "core/EventBus.hpp"
#include "model/build/BuildingSystem.hpp"
#include "model/combat/DamageSystem.hpp"
#include "model/combat/ProjectileSystem.hpp"
#include "model/units/UnitSystem.hpp"
#include "model/ecs/Registry.hpp"

#include <functional>
#include <utility>
#include <vector>

namespace model {

class CombatSystem {
public:
    CombatSystem(core::EventBus& bus, ecs::Registry& world, UnitSystem& units, BuildingSystem& buildings,
        DamageSystem& damage, ProjectileSystem& projectiles);
    void reset();
    void orderAttack(PlayerId player, const std::vector<ecs::Entity>& attackers, ecs::Entity target);
    void update(float deltaSeconds);

    // Extra tiles that block direct fire (typically terrain). The default only blocks buildings.
    void setTileSightBlocker(std::function<bool(GridPos)> blocker)
    {
        tileSightBlocker_ = std::move(blocker);
    }

private:
    // Enemy entities are valid targets; a Training Dummy is valid even for its owner.
    [[nodiscard]] bool validTarget(PlayerId player, ecs::Entity target) const;
    // Units need a weapon and at least one round of ammo; other combat entities do not.
    [[nodiscard]] bool canFire(ecs::Entity entity) const;
    // Direct shots need an unobstructed line between the shooter and the target.
    [[nodiscard]] bool hasLineOfSightTo(ecs::Entity source, ecs::Entity target) const;
    // Tile used as the center of the target footprint when walking into weapon range.
    [[nodiscard]] GridPos targetTile(ecs::Entity target) const;
    void consumeAmmo(ecs::Entity entity) const;

    core::EventBus& bus_;
    ecs::Registry& world_;
    UnitSystem& units_;
    BuildingSystem& buildings_;
    DamageSystem& damage_;
    ProjectileSystem& projectiles_;
    std::function<bool(GridPos)> tileSightBlocker_;
};

} // namespace model
