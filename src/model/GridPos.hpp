#pragma once

#include <cstddef>
#include <functional>

namespace model {

// Logical tile coordinate. Knows nothing about pixels or the isometric projection.
struct GridPos {
    int x = 0;
    int y = 0;

    friend constexpr bool operator==(const GridPos&, const GridPos&) = default;
};

struct GridPosHash {
    [[nodiscard]] std::size_t operator()(const GridPos& pos) const noexcept
    {
        const std::size_t hx = std::hash<int>{}(pos.x);
        const std::size_t hy = std::hash<int>{}(pos.y);
        return hx ^ (hy + 0x9e3779b9u + (hx << 6) + (hx >> 2));
    }
};

} // namespace model
