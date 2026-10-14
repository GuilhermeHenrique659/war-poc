#pragma once

#include "model/GridPos.hpp"
#include "model/UnitType.hpp"
#include "model/components/Owner.hpp"

#include <cstddef>
#include <vector>

namespace model::components {

struct Unit {
    UnitType type;
    int baseCount; // Recruited units inside it (merges add up).
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

} // namespace model::components
