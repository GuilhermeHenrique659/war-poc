#pragma once

#include "view/gameplay/TimeHudLayout.hpp"

#include <raylib.h>

namespace view::player_hud_layout {

inline constexpr float kWidth = 170.0F;
inline constexpr float kHeight = 72.0F;
inline constexpr float kGap = 10.0F;
// The calendar card occupies the top-right corner; the player card sits below it.
inline constexpr float kTop = time_hud_layout::kTop + time_hud_layout::kHeight + kGap;

[[nodiscard]] inline Rectangle panelBounds(int screenWidth)
{
    return Rectangle{static_cast<float>(screenWidth) - kWidth - kGap, kTop, kWidth, kHeight};
}

} // namespace view::player_hud_layout
