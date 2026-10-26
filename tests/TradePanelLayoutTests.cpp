#include "view/economy/TradePanelLayout.hpp"

#include <gtest/gtest.h>

TEST(TradePanelLayoutTests, PanelGrowsWithTheNumberOfRows)
{
    // Given the same screen width.
    constexpr int kScreenWidth = 1920;

    // When the panel shows eight rows instead of three.
    const Rectangle eight = view::trade_panel_layout::panelBounds(8, kScreenWidth);
    const Rectangle three = view::trade_panel_layout::panelBounds(3, kScreenWidth);

    // Then it grows by exactly five rows.
    EXPECT_FLOAT_EQ(eight.height - three.height, 5.0F * view::trade_panel_layout::kRowHeight);
}

TEST(TradePanelLayoutTests, RowsStackInsideThePanelWithoutOverlap)
{
    // Given a panel with eight rows.
    constexpr int kScreenWidth = 1920;
    const Rectangle panel = view::trade_panel_layout::panelBounds(8, kScreenWidth);

    // Then the rows are stacked one row apart and stay inside the panel.
    const Rectangle first = view::trade_panel_layout::rowBounds(0, 8, kScreenWidth);
    const Rectangle second = view::trade_panel_layout::rowBounds(1, 8, kScreenWidth);
    EXPECT_GE(first.y, panel.y);
    EXPECT_FLOAT_EQ(second.y - first.y, view::trade_panel_layout::kRowHeight);
    EXPECT_LE(second.y + second.height, panel.y + panel.height);
}

TEST(TradePanelLayoutTests, CloseButtonSitsInsideTheHeader)
{
    // Given a panel with eight rows.
    constexpr int kScreenWidth = 1920;
    const Rectangle panel = view::trade_panel_layout::panelBounds(8, kScreenWidth);

    // Then the close button is a square on the right side of the header.
    const Rectangle close = view::trade_panel_layout::closeBounds(8, kScreenWidth);
    EXPECT_FLOAT_EQ(close.width, view::trade_panel_layout::kCloseSize);
    EXPECT_FLOAT_EQ(close.height, view::trade_panel_layout::kCloseSize);
    EXPECT_GT(close.x, panel.x + panel.width / 2.0F);
    EXPECT_LE(close.x + close.width, panel.x + panel.width);
    EXPECT_GE(close.y, panel.y);
    EXPECT_LE(close.y + close.height, panel.y + view::trade_panel_layout::kPadding
        + view::trade_panel_layout::kHeaderHeight);
}
