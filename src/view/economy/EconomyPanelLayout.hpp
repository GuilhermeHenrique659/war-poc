#pragma once

#include <raylib.h>

namespace view::economy_panel_layout {

inline constexpr float kWidth = 210.0f;
inline constexpr float kHeight = 156.0f;
inline constexpr float kMargin = 10.0f;
inline constexpr float kPadding = 10.0f;

[[nodiscard]] inline Rectangle panelBounds(int screenWidth)
{
    return Rectangle{static_cast<float>(screenWidth) - kWidth - kMargin, kMargin, kWidth, kHeight};
}

} // namespace view::economy_panel_layout
