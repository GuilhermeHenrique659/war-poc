#pragma once

#include <raylib.h>

#include <cstddef>

namespace view::inventory_panel_layout {

inline constexpr float kWidth = 620.0F;
inline constexpr float kHeight = 430.0F;
inline constexpr float kSlotSize = 30.0F;
inline constexpr float kSlotSpacing = 2.0F;
inline constexpr float kPadding = 8.0F;
inline constexpr std::size_t kColumns = 8;
inline constexpr float kPaneWidth = 300.0F;
inline constexpr float kGridTop = 112.0F;

[[nodiscard]] inline Rectangle panelBounds(int screenWidth)
{
    return Rectangle{(static_cast<float>(screenWidth) - kWidth) / 2.0F, 10.0F, kWidth, kHeight};
}

[[nodiscard]] inline Rectangle closeBounds(int screenWidth)
{
    const Rectangle panel = panelBounds(screenWidth);
    return Rectangle{panel.x + panel.width - 30.0F, panel.y + 5.0F, 22.0F, 22.0F};
}

[[nodiscard]] inline Rectangle paneBounds(bool playerInventory, int screenWidth)
{
    const Rectangle panel = panelBounds(screenWidth);
    const float x = panel.x + kPadding + (playerInventory ? 0.0F : kPaneWidth);
    return Rectangle{x, panel.y + kGridTop - 20.0F, kPaneWidth - kPadding, panel.height - kGridTop + 12.0F};
}

[[nodiscard]] inline Rectangle slotBounds(std::size_t index, bool playerInventory, int screenWidth)
{
    const Rectangle pane = paneBounds(playerInventory, screenWidth);
    const std::size_t row = index / kColumns;
    const std::size_t column = index % kColumns;
    const float x = pane.x + static_cast<float>(column) * (kSlotSize + kSlotSpacing);
    const float y = pane.y + 20.0F + static_cast<float>(row) * (kSlotSize + kSlotSpacing);
    return Rectangle{x, y, kSlotSize, kSlotSize};
}

} // namespace view::inventory_panel_layout
