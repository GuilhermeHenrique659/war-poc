#pragma once

#include <raylib.h>

namespace view::player_hud_layout {

inline constexpr float kWidth = 170.0F;
inline constexpr float kHeight = 72.0F;
inline constexpr float kGap = 10.0F;

[[nodiscard]] inline Rectangle panelBounds(int screenWidth)
{
    return Rectangle{static_cast<float>(screenWidth) - kWidth - kGap, kGap, kWidth, kHeight};
}

} // namespace view::player_hud_layout
