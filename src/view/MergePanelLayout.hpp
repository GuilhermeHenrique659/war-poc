#pragma once

#include "view/EconomyPanelLayout.hpp"

#include <raylib.h>

// Shared merge panel geometry: the view draws with it, the controller hit-tests with it.
// Same corner as the recruit panel; they are never open together.
namespace view::merge_panel_layout {

inline constexpr float kWidth = 290.0f;
inline constexpr float kMargin = 10.0f;
inline constexpr float kTop = economy_panel_layout::kMargin + economy_panel_layout::kHeight + 10.0f;
inline constexpr float kPadding = 10.0f;
inline constexpr float kHeaderHeight = 36.0f;
inline constexpr float kLineHeight = 18.0f;
inline constexpr int kStatLines = 5;
inline constexpr float kButtonSpacing = 8.0f;
inline constexpr float kButtonHeight = 44.0f;

[[nodiscard]] inline Rectangle panelBounds(int screenWidth)
{
    const float height = 2.0f * kPadding + kHeaderHeight + static_cast<float>(kStatLines) * kLineHeight
        + kButtonSpacing + kButtonHeight;
    return Rectangle{static_cast<float>(screenWidth) - kWidth - kMargin, kTop, kWidth, height};
}

[[nodiscard]] inline Rectangle buttonBounds(int screenWidth)
{
    const Rectangle panel = panelBounds(screenWidth);
    const float y = panel.y + panel.height - kPadding - kButtonHeight;
    return Rectangle{panel.x + kPadding, y, panel.width - 2.0f * kPadding, kButtonHeight};
}

} // namespace view::merge_panel_layout
