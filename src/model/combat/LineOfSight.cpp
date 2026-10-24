#include "model/combat/LineOfSight.hpp"

#include <algorithm>
#include <limits>

namespace model::combat {

bool hasLineOfSight(GridPos from, GridPos to, const TileBlocker& blocksSight)
{
    const int dx = to.x - from.x;
    const int dy = to.y - from.y;
    if (dx == 0 && dy == 0) {
        return true;
    }
    if (!blocksSight) {
        return true;
    }

    const int stepX = (dx > 0) - (dx < 0);
    const int stepY = (dy > 0) - (dy < 0);
    const int distanceX = std::abs(dx);
    const int distanceY = std::abs(dy);
    const float infinite = std::numeric_limits<float>::infinity();

    // Amanatides & Woo traversal: tMax is the ray parameter of the next tile boundary crossing.
    float nextX = distanceX == 0 ? infinite : 0.5F / static_cast<float>(distanceX);
    float nextY = distanceY == 0 ? infinite : 0.5F / static_cast<float>(distanceY);
    const float deltaX = distanceX == 0 ? infinite : 1.0F / static_cast<float>(distanceX);
    const float deltaY = distanceY == 0 ? infinite : 1.0F / static_cast<float>(distanceY);

    int x = from.x;
    int y = from.y;
    while (x != to.x || y != to.y) {
        if (nextX < nextY) {
            x += stepX;
            nextX += deltaX;
        } else if (nextY < nextX) {
            y += stepY;
            nextY += deltaY;
        } else {
            // Exact corner crossing: the diagonal neighbour is the only tile on the line.
            x += stepX;
            y += stepY;
            nextX += deltaX;
            nextY += deltaY;
        }
        if (x == to.x && y == to.y) {
            return true;
        }
        if (blocksSight(GridPos{x, y})) {
            return false;
        }
    }
    return true;
}

} // namespace model::combat
