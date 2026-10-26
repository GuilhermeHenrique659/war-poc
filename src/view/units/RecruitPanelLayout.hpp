#pragma once

#include "view/player/PlayerHudLayout.hpp"

#include <raylib.h>

#include <cstddef>

// Shared recruit panel geometry: the view draws with it, the controller hit-tests with it.
namespace view::recruit_panel_layout {

inline constexpr float kWidth = 290.0f;
inline constexpr float kMargin = 10.0f;
inline constexpr float kTop =
    player_hud_layout::kTop + player_hud_layout::kHeight + player_hud_layout::kGap;
inline constexpr float kPadding = 10.0f;
inline constexpr float kHeaderHeight = 36.0f;
inline constexpr float kSlotHeight = 70.0f;
inline constexpr float kSlotSpacing = 8.0f;
inline constexpr float kCloseSize = 24.0f;

[[nodiscard]] inline Rectangle panelBounds(std::size_t count, int screenWidth)
{
    const float slots = static_cast<float>(count) * kSlotHeight
        + static_cast<float>(count > 0 ? count - 1 : 0) * kSlotSpacing;
    const float height = kHeaderHeight + slots + 2.0f * kPadding;
    return Rectangle{static_cast<float>(screenWidth) - kWidth - kMargin, kTop, kWidth, height};
}

[[nodiscard]] inline Rectangle closeBounds(std::size_t count, int screenWidth)
{
    const Rectangle panel = panelBounds(count, screenWidth);
    return Rectangle{panel.x + panel.width - kPadding - kCloseSize, panel.y + (kHeaderHeight - kCloseSize) / 2.0f + 4.0f,
        kCloseSize, kCloseSize};
}

[[nodiscard]] inline Rectangle slotBounds(std::size_t index, std::size_t count, int screenWidth)
{
    const Rectangle panel = panelBounds(count, screenWidth);
    const float y = panel.y + kPadding + kHeaderHeight + static_cast<float>(index) * (kSlotHeight + kSlotSpacing);
    return Rectangle{panel.x + kPadding, y, panel.width - 2.0f * kPadding, kSlotHeight};
}

} // namespace view::recruit_panel_layout
