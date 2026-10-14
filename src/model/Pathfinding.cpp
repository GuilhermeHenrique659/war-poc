#include "model/Pathfinding.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <deque>
#include <queue>
#include <unordered_map>
#include <unordered_set>

namespace model::pathfinding {

namespace {

// The map is infinite, so both searches need a hard budget.
constexpr std::size_t kMaxExpansions = 20000;
constexpr std::size_t kMaxVisited = 10000;
constexpr float kDiagonalCost = 1.41421356f;

constexpr std::array<GridPos, 8> kDirections{
    GridPos{1, 0}, GridPos{-1, 0}, GridPos{0, 1}, GridPos{0, -1},
    GridPos{1, 1}, GridPos{1, -1}, GridPos{-1, 1}, GridPos{-1, -1},
};

struct OpenNode {
    float estimate; // Cost so far plus heuristic.
    float cost;
    GridPos tile;

    friend bool operator>(const OpenNode& a, const OpenNode& b) { return a.estimate > b.estimate; }
};

[[nodiscard]] float octile(GridPos a, GridPos b)
{
    const float dx = static_cast<float>(std::abs(a.x - b.x));
    const float dy = static_cast<float>(std::abs(a.y - b.y));
    return dx + dy + (kDiagonalCost - 2.0f) * std::min(dx, dy);
}

[[nodiscard]] std::vector<GridPos> rebuild(
    const std::unordered_map<GridPos, GridPos, GridPosHash>& cameFrom, GridPos start, GridPos goal)
{
    std::vector<GridPos> path;
    for (GridPos tile = goal; tile != start; tile = cameFrom.at(tile)) {
        path.push_back(tile);
    }
    std::reverse(path.begin(), path.end());
    return path;
}

} // namespace

std::optional<std::vector<GridPos>> findPath(GridPos start, GridPos goal, const TilePredicate& blocked)
{
    if (start == goal) {
        return std::vector<GridPos>{};
    }
    if (blocked(goal)) {
        return std::nullopt;
    }

    std::priority_queue<OpenNode, std::vector<OpenNode>, std::greater<>> open;
    std::unordered_map<GridPos, float, GridPosHash> bestCost{{start, 0.0f}};
    std::unordered_map<GridPos, GridPos, GridPosHash> cameFrom;
    open.push(OpenNode{octile(start, goal), 0.0f, start});

    std::size_t expansions = 0;
    while (!open.empty()) {
        const OpenNode current = open.top();
        open.pop();
        if (current.cost > bestCost.at(current.tile)) {
            continue; // Stale entry.
        }
        if (current.tile == goal) {
            return rebuild(cameFrom, start, goal);
        }
        if (++expansions > kMaxExpansions) {
            return std::nullopt;
        }

        for (const GridPos dir : kDirections) {
            const GridPos next{current.tile.x + dir.x, current.tile.y + dir.y};
            if (blocked(next)) {
                continue;
            }
            const bool diagonal = dir.x != 0 && dir.y != 0;
            if (diagonal
                && (blocked(GridPos{current.tile.x + dir.x, current.tile.y})
                    || blocked(GridPos{current.tile.x, current.tile.y + dir.y}))) {
                continue;
            }

            const float cost = current.cost + (diagonal ? kDiagonalCost : 1.0f);
            const auto known = bestCost.find(next);
            if (known != bestCost.end() && known->second <= cost) {
                continue;
            }
            bestCost[next] = cost;
            cameFrom[next] = current.tile;
            open.push(OpenNode{cost + octile(next, goal), cost, next});
        }
    }
    return std::nullopt;
}

std::vector<GridPos> nearestFreeTiles(GridPos center, std::size_t count, const TilePredicate& unavailable)
{
    std::vector<GridPos> result;
    if (count == 0) {
        return result;
    }

    std::deque<GridPos> frontier{center};
    std::unordered_set<GridPos, GridPosHash> visited{center};
    while (!frontier.empty() && result.size() < count && visited.size() < kMaxVisited) {
        const GridPos tile = frontier.front();
        frontier.pop_front();
        if (!unavailable(tile)) {
            result.push_back(tile);
        }
        for (const GridPos dir : kDirections) {
            const GridPos next{tile.x + dir.x, tile.y + dir.y};
            if (visited.insert(next).second) {
                frontier.push_back(next);
            }
        }
    }
    return result;
}

} // namespace model::pathfinding
