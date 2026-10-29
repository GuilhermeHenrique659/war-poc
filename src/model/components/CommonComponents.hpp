#pragma once

#include "model/Direction.hpp"
#include "model/GridPos.hpp"
#include "model/components/Owner.hpp"

#include <cstddef>
#include <vector>

namespace model::components {

// Continuous grid coordinates of the entity's center.
struct Position {
    float x;
    float y;
};

struct Health {
    int current;
    int max;
};

// Direction the entity is looking towards. Updated by the movement systems and published to the
// View whenever it changes.
struct Facing {
    model::Direction direction = model::Direction::South;
};

// Speed and route of an entity that walks along grid paths. The waypoint still ahead starts at
// `next`; `destination` is the origin of the square reserved for the entity.
struct Mover {
    float speed; // Tiles per second.
    std::vector<GridPos> path;
    std::size_t next;
    GridPos destination;
};

// Square footprint of an entity that occupies tiles.
struct Footprint {
    GridPos origin;
    int size;
};

} // namespace model::components
