#pragma once

#include <raylib.h>

#include <algorithm>
#include <cstddef>

// Shared build bar geometry: the view draws with it, the controller hit-tests with it.
namespace view::build_bar_layout {

inline constexpr float kSlotWidth = 200.0f;
inline constexpr float kSlotHeight = 64.0f;
inline constexpr float kSlotSpacing = 10.0f;
inline constexpr float kPadding = 10.0f;
[[nodiscard]] inline int columnsFor(int screenWidth, std::size_t count)
{
    if (count == 0) {
        return 1;
    }
    const float available = static_cast<float>(screenWidth) - 2.0f * kPadding;
    const int fitting = static_cast<int>((available + kSlotSpacing) / (kSlotWidth + kSlotSpacing));
    return std::max(1, std::min(fitting, static_cast<int>(count)));
}

[[nodiscard]] inline int rowCount(int screenWidth, std::size_t count)
{
    const int columns = columnsFor(screenWidth, count);
    const int items = std::max(1, static_cast<int>(count));
    return (items + columns - 1) / columns;
}

[[nodiscard]] inline Rectangle barBounds(int screenWidth, int screenHeight, std::size_t count)
{
    const int rows = rowCount(screenWidth, count);
    const float barHeight = 2.0f * kPadding + static_cast<float>(rows) * kSlotHeight
        + static_cast<float>(rows - 1) * kSlotSpacing;
    const float height = static_cast<float>(screenHeight);
    return Rectangle{0.0f, height - barHeight, static_cast<float>(screenWidth), barHeight};
}

[[nodiscard]] inline Rectangle slotBounds(std::size_t index, int screenWidth, int screenHeight, std::size_t count)
{
    const Rectangle bar = barBounds(screenWidth, screenHeight, count);
    const int columns = columnsFor(screenWidth, count);
    const int row = static_cast<int>(index) / columns;
    const int column = static_cast<int>(index) % columns;
    const float x = bar.x + kPadding + static_cast<float>(column) * (kSlotWidth + kSlotSpacing);
    const float y = bar.y + kPadding + static_cast<float>(row) * (kSlotHeight + kSlotSpacing);
    return Rectangle{x, y, kSlotWidth, kSlotHeight};
}

} // namespace view::build_bar_layout
