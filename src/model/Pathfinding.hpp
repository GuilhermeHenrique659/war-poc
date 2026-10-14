#pragma once

#include "model/GridPos.hpp"

#include <cstddef>
#include <functional>
#include <optional>
#include <vector>

namespace model::pathfinding {

using TilePredicate = std::function<bool(GridPos)>;

// 8-directional A* that never cuts corners. The path excludes `start` and ends at `goal`.
// Returns nothing when the goal is blocked or not reached within the search budget.
[[nodiscard]] std::optional<std::vector<GridPos>> findPath(GridPos start, GridPos goal, const TilePredicate& blocked);

inline constexpr int kUnlimitedRadius = -1;

// Up to `count` tiles for which `unavailable` is false, nearest to `center` first (square rings).
// With a radius, only tiles up to that many rings away are considered.
[[nodiscard]] std::vector<GridPos> nearestFreeTiles(
    GridPos center, std::size_t count, const TilePredicate& unavailable, int maxRadius = kUnlimitedRadius);

} // namespace model::pathfinding
