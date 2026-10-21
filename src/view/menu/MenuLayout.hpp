#pragma once

#include <raylib.h>

#include <cstddef>

// Shared menu geometry: the view draws with it, the controller hit-tests with it.
namespace view::menu_layout {

inline constexpr float kButtonWidth = 260.0f;
inline constexpr float kButtonHeight = 56.0f;
inline constexpr float kButtonSpacing = 20.0f;
inline constexpr float kTitleOffsetY = 140.0f;

[[nodiscard]] inline Rectangle itemBounds(std::size_t index, std::size_t count, int screenWidth, int screenHeight)
{
    const float total = static_cast<float>(count) * kButtonHeight
        + static_cast<float>(count > 0 ? count - 1 : 0) * kButtonSpacing;
    const float startY = (static_cast<float>(screenHeight) - total) / 2.0f + kButtonHeight;
    const float x = (static_cast<float>(screenWidth) - kButtonWidth) / 2.0f;
    const float y = startY + static_cast<float>(index) * (kButtonHeight + kButtonSpacing);
    return Rectangle{x, y, kButtonWidth, kButtonHeight};
}

} // namespace view::menu_layout
