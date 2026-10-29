#pragma once

#include "model/components/CommonComponents.hpp"
#include "model/units/UnitType.hpp"
#include "model/ecs/Entity.hpp"

#include <cstddef>
#include <optional>

namespace model {

inline constexpr std::size_t kUnitInventorySlots = 2;
inline constexpr int kUnitWeaponCount = 1;
inline constexpr int kUnitAmmoCapacity = 100;
// A unit counts as arrived when it is this close (in tiles) to the target footprint.
inline constexpr float kUnitArrivalRadius = 2.5F;

// What the unit is doing on its own. Player orders still move it while it is Idle.
enum class UnitActivity {
    Idle,
    SeekingSupplies,
    WaitingForSupplies,
};

} // namespace model

namespace model::components {

struct Unit {
    UnitType type;
    int baseCount; // Recruited units represented by this unit.
    int size; // Square footprint, in tiles per side.
};

struct AttackOrder {
    ecs::Entity target;
    float cooldownRemaining;
};

// State machine driving the automatic resupply behaviour.
struct UnitSupplyState {
    UnitActivity activity = UnitActivity::Idle;
    std::optional<ecs::Entity> supplyTarget;
};

} // namespace model::components
