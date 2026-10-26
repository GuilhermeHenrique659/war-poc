#pragma once

#include "view/player/PlayerHudLayout.hpp"

#include <raylib.h>

#include <cstddef>

namespace view::trade_panel_layout {

inline constexpr float kWidth = 400.0F;
inline constexpr float kMargin = 10.0F;
inline constexpr float kTop =
    player_hud_layout::kTop + player_hud_layout::kHeight + player_hud_layout::kGap;
inline constexpr float kPadding = 10.0F;
inline constexpr float kHeaderHeight = 34.0F;
inline constexpr float kRowHeight = 26.0F;
inline constexpr float kCloseSize = 24.0F;

[[nodiscard]] inline Rectangle panelBounds(std::size_t rowCount, int screenWidth)
{
    const float rowsHeight = static_cast<float>(rowCount) * kRowHeight;
    const float height = kHeaderHeight + rowsHeight + 2.0F * kPadding;
    return Rectangle{static_cast<float>(screenWidth) - kWidth - kMargin, kTop, kWidth, height};
}

[[nodiscard]] inline Rectangle closeBounds(std::size_t rowCount, int screenWidth)
{
    const Rectangle panel = panelBounds(rowCount, screenWidth);
    return Rectangle{panel.x + panel.width - kPadding - kCloseSize,
        panel.y + (kHeaderHeight - kCloseSize) / 2.0F, kCloseSize, kCloseSize};
}

[[nodiscard]] inline Rectangle rowBounds(std::size_t index, std::size_t rowCount, int screenWidth)
{
    const Rectangle panel = panelBounds(rowCount, screenWidth);
    const float y = panel.y + kPadding + kHeaderHeight + static_cast<float>(index) * kRowHeight;
    return Rectangle{panel.x + kPadding, y, panel.width - 2.0F * kPadding, kRowHeight - 4.0F};
}

} // namespace view::trade_panel_layout
