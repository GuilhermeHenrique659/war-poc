#pragma once

#include <raylib.h>

#include <cstddef>

namespace view::logistics_panel_layout {

inline constexpr float kWidth = 350.0F;
inline constexpr float kMargin = 10.0F;
inline constexpr float kTop = 92.0F;
inline constexpr float kPadding = 10.0F;
inline constexpr float kHeaderHeight = 34.0F;
inline constexpr float kRecruitHeight = 48.0F;
inline constexpr float kRouteHeight = 56.0F;
inline constexpr float kEmptyHeight = 34.0F;
inline constexpr float kCloseSize = 24.0F;
inline constexpr float kActionWidth = 76.0F;
inline constexpr float kActionGap = 4.0F;

[[nodiscard]] inline Rectangle panelBounds(std::size_t routeCount, bool warehouse, int screenWidth)
{
    const float actionHeight = warehouse ? kRecruitHeight : 0.0F;
    const float rowsHeight = routeCount == 0 ? kEmptyHeight : static_cast<float>(routeCount) * kRouteHeight;
    const float height = kHeaderHeight + actionHeight + rowsHeight + 2.0F * kPadding;
    return Rectangle{static_cast<float>(screenWidth) - kWidth - kMargin, kTop, kWidth, height};
}

[[nodiscard]] inline Rectangle closeBounds(std::size_t routeCount, bool warehouse, int screenWidth)
{
    const Rectangle panel = panelBounds(routeCount, warehouse, screenWidth);
    return Rectangle{panel.x + panel.width - kPadding - kCloseSize,
        panel.y + (kHeaderHeight - kCloseSize) / 2.0F, kCloseSize, kCloseSize};
}

[[nodiscard]] inline Rectangle recruitBounds(std::size_t routeCount, int screenWidth)
{
    const Rectangle panel = panelBounds(routeCount, true, screenWidth);
    const float y = panel.y + kPadding + kHeaderHeight;
    return Rectangle{panel.x + kPadding, y, panel.width - 2.0F * kPadding, kRecruitHeight - 6.0F};
}

[[nodiscard]] inline Rectangle routeBounds(std::size_t index, std::size_t routeCount, int screenWidth)
{
    const Rectangle panel = panelBounds(routeCount, false, screenWidth);
    const float y = panel.y + kPadding + kHeaderHeight + static_cast<float>(index) * kRouteHeight;
    return Rectangle{panel.x + kPadding, y, panel.width - 2.0F * kPadding, kRouteHeight - 4.0F};
}

[[nodiscard]] inline Rectangle assignBounds(std::size_t index, std::size_t routeCount, int screenWidth)
{
    const Rectangle row = routeBounds(index, routeCount, screenWidth);
    return Rectangle{row.x + row.width - 2.0F * kActionWidth - kActionGap,
        row.y + 7.0F, kActionWidth, row.height - 14.0F};
}

[[nodiscard]] inline Rectangle unassignBounds(std::size_t index, std::size_t routeCount, int screenWidth)
{
    const Rectangle row = routeBounds(index, routeCount, screenWidth);
    return Rectangle{row.x + row.width - kActionWidth, row.y + 7.0F,
        kActionWidth, row.height - 14.0F};
}

} // namespace view::logistics_panel_layout
