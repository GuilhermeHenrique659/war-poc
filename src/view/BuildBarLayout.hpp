#pragma once

#include <raylib.h>

#include <cstddef>

// Shared build bar geometry: the view draws with it, the controller hit-tests with it.
namespace view::build_bar_layout {

inline constexpr float kSlotWidth = 200.0f;
inline constexpr float kSlotHeight = 64.0f;
inline constexpr float kSlotSpacing = 10.0f;
inline constexpr float kPadding = 10.0f;
inline constexpr float kBarHeight = kSlotHeight + 2.0f * kPadding;

[[nodiscard]] inline Rectangle barBounds(int screenWidth, int screenHeight)
{
    const float height = static_cast<float>(screenHeight);
    return Rectangle{0.0f, height - kBarHeight, static_cast<float>(screenWidth), kBarHeight};
}

[[nodiscard]] inline Rectangle slotBounds(std::size_t index, int screenWidth, int screenHeight)
{
    const Rectangle bar = barBounds(screenWidth, screenHeight);
    const float x = bar.x + kPadding + static_cast<float>(index) * (kSlotWidth + kSlotSpacing);
    return Rectangle{x, bar.y + kPadding, kSlotWidth, kSlotHeight};
}

} // namespace view::build_bar_layout
