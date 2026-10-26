#pragma once

#include <raylib.h>

namespace view::time_hud_layout {

inline constexpr float kWidth = 170.0F;
inline constexpr float kHeight = 54.0F;
inline constexpr float kMargin = 10.0F;
inline constexpr float kTop = 10.0F;

[[nodiscard]] inline Rectangle panelBounds(int screenWidth)
{
    return Rectangle{static_cast<float>(screenWidth) - kWidth - kMargin, kTop, kWidth, kHeight};
}

} // namespace view::time_hud_layout
