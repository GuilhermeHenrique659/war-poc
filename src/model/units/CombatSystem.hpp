#pragma once

#include "core/EventBus.hpp"
#include "model/build/BuildingSystem.hpp"
#include "model/units/UnitSystem.hpp"
#include "model/ecs/Registry.hpp"

#include <vector>

namespace model {

class CombatSystem {
public:
    CombatSystem(core::EventBus& bus, ecs::Registry& world, UnitSystem& units, BuildingSystem& buildings);
    void reset();
    void orderAttack(PlayerId player, const std::vector<ecs::Entity>& attackers, ecs::Entity target);
    void update(float deltaSeconds);

private:
    [[nodiscard]] bool validTarget(PlayerId player, ecs::Entity target) const;

    core::EventBus& bus_;
    ecs::Registry& world_;
    UnitSystem& units_;
    BuildingSystem& buildings_;
};

} // namespace model
