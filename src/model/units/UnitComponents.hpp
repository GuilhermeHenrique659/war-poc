#pragma once

#include "model/GridPos.hpp"
#include "model/units/UnitType.hpp"
#include "model/components/Owner.hpp"
#include "model/ecs/Entity.hpp"

#include <cstddef>
#include <optional>
#include <vector>

namespace model {

inline constexpr std::size_t kUnitInventorySlots = 2;
inline constexpr int kUnitWeaponCount = 1;
inline constexpr int kUnitAmmoCapacity = 100;

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

struct Health {
    int current;
    int max;
};

struct Combat {
    int damage;
    float range; // Tiles.
};

struct AttackOrder {
    ecs::Entity target;
    float cooldownRemaining;
};

// Continuous grid coordinates of the unit's center. A unit of size N whose square starts at
// tile (x, y) is centered at (x + N / 2, y + N / 2).
struct Position {
    float x;
    float y;
};

struct Mover {
    float speed; // Tiles per second.
    std::vector<GridPos> path; // Square origins still ahead start at `next`.
    std::size_t next;
    GridPos destination; // Origin of the square reserved for this unit (where it stands when idle).
};

// State machine driving the automatic resupply behaviour.
struct UnitSupplyState {
    UnitActivity activity = UnitActivity::Idle;
    std::optional<ecs::Entity> supplyTarget;
};

} // namespace model::components
