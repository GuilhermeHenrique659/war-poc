#pragma once

#include "model/GridPos.hpp"
#include "model/UnitType.hpp"
#include "model/components/Owner.hpp"

#include <cstddef>
#include <vector>

namespace model::components {

struct Unit {
    UnitType type;
};

struct Health {
    int current;
    int max;
};

struct Combat {
    int damage;
    float range; // Tiles.
};

// Continuous grid coordinates. The center of tile (x, y) is (x + 0.5, y + 0.5).
struct Position {
    float x;
    float y;
};

struct Mover {
    float speed; // Tiles per second.
    std::vector<GridPos> path; // Waypoints still ahead start at `next`.
    std::size_t next;
    GridPos destination; // Tile reserved for this unit (where it stands when idle).
};

} // namespace model::components
