#pragma once

#include "model/GridPos.hpp"

#include <functional>

namespace model::combat {

using TileBlocker = std::function<bool(GridPos)>;

// True when the straight segment between the two tile centres crosses no blocked tile.
// The end tiles are never tested: they belong to the shooter and to the target themselves.
[[nodiscard]] bool hasLineOfSight(GridPos from, GridPos to, const TileBlocker& blocksSight);

} // namespace model::combat
