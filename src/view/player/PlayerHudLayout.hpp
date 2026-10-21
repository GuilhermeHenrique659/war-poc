#pragma once

#include "view/economy/EconomyPanelLayout.hpp"

#include <raylib.h>

namespace view::player_hud_layout {

inline constexpr float kWidth = 170.0F;
inline constexpr float kHeight = 72.0F;
inline constexpr float kGap = 10.0F;

[[nodiscard]] inline Rectangle panelBounds(int screenWidth)
{
    const Rectangle economy = economy_panel_layout::panelBounds(screenWidth);
    return Rectangle{economy.x - kWidth - kGap, economy.y, kWidth, kHeight};
}

} // namespace view::player_hud_layout
