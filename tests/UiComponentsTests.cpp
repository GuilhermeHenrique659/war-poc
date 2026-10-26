#include "view/ui/Bar.hpp"
#include "view/ui/Button.hpp"
#include "view/ui/Swatch.hpp"
#include "view/ui/Text.hpp"
#include "view/ui/TextBlock.hpp"

#include <gtest/gtest.h>
#include <raylib.h>

#include <array>
#include <string>
#include <string_view>
#include <vector>

namespace {

struct FilledRect {
    Rectangle bounds{};
    Color color{};
};

struct OutlinedRect {
    Rectangle bounds{};
    float thickness = 0.0F;
    Color color{};
};

struct DrawnText {
    std::string text;
    int x = 0;
    int y = 0;
    int fontSize = 0;
    Color color{};
};

constexpr Color kRed{230, 41, 55, 255};
constexpr Color kGreen{0, 228, 48, 255};
constexpr Color kBlack{0, 0, 0, 255};
constexpr Color kWhite{255, 255, 255, 255};

std::vector<FilledRect> filledRects;
std::vector<OutlinedRect> outlinedRects;
std::vector<DrawnText> drawnTexts;

void resetDrawCalls()
{
    filledRects.clear();
    outlinedRects.clear();
    drawnTexts.clear();
}

} // namespace

void DrawRectangleRec(Rectangle rec, Color color)
{
    filledRects.push_back(FilledRect{rec, color});
}

void DrawRectangleLinesEx(Rectangle rec, float lineThick, Color color)
{
    outlinedRects.push_back(OutlinedRect{rec, lineThick, color});
}

int MeasureText(const char* text, int fontSize)
{
    return static_cast<int>(std::string_view{text}.size()) * fontSize;
}

void DrawText(const char* text, int posX, int posY, int fontSize, Color color)
{
    drawnTexts.push_back(DrawnText{text, posX, posY, fontSize, color});
}

namespace view::ui {
namespace {

class UiComponentsTests : public ::testing::Test {
protected:
    void SetUp() override
    {
        resetDrawCalls();
    }
};

TEST_F(UiComponentsTests, TextDrawsAtTheTopLeftOfItsBoundsByDefault)
{
    // Given a red 20px text anchored at the top-left.
    const Text text{TextStyle{20, kRed}};

    // When it is drawn inside a rectangle.
    text.draw(Rectangle{10.0F, 20.0F, 100.0F, 40.0F}, "abc");

    // Then raylib receives the top-left position, size and color.
    ASSERT_EQ(drawnTexts.size(), 1U);
    EXPECT_EQ(drawnTexts[0].text, "abc");
    EXPECT_EQ(drawnTexts[0].x, 10);
    EXPECT_EQ(drawnTexts[0].y, 20);
    EXPECT_EQ(drawnTexts[0].fontSize, 20);
    EXPECT_EQ(drawnTexts[0].color.r, kRed.r);
    EXPECT_EQ(drawnTexts[0].color.g, kRed.g);
    EXPECT_EQ(drawnTexts[0].color.b, kRed.b);
}

TEST_F(UiComponentsTests, TextCentersHorizontallyAndVerticallyInsideItsBounds)
{
    // Given a 20px text centered in a 100x40 rectangle.
    const Text text{TextStyle{20, kWhite, TextAlign::Center, TextVerticalAlign::Middle}};

    // When it is drawn.
    text.draw(Rectangle{10.0F, 20.0F, 100.0F, 40.0F}, "abc");

    // Then the measured width and font height are used for centering.
    ASSERT_EQ(drawnTexts.size(), 1U);
    EXPECT_EQ(drawnTexts[0].x, 30);
    EXPECT_EQ(drawnTexts[0].y, 30);
}

TEST_F(UiComponentsTests, TextAlignsRightAndBottomInsideItsBounds)
{
    // Given a text anchored at the right/bottom edge.
    const Text text{TextStyle{20, kWhite, TextAlign::Right, TextVerticalAlign::Bottom}};

    // When it is drawn.
    text.draw(Rectangle{10.0F, 20.0F, 100.0F, 40.0F}, "abc");

    // Then the right and bottom edges are respected.
    ASSERT_EQ(drawnTexts.size(), 1U);
    EXPECT_EQ(drawnTexts[0].x, 50);
    EXPECT_EQ(drawnTexts[0].y, 40);
}

TEST_F(UiComponentsTests, BarDrawsFrameBackgroundFillAndBorder)
{
    // Given a half-filled bar with a frame, background, fill and border.
    const Bar bar{BarStyle{
        Color{10, 10, 10, 255},
        Color{20, 200, 20, 255},
        Color{200, 200, 200, 255},
        Color{0, 0, 0, 255},
        1.0F,
        2.0F,
    }};

    // When it is drawn.
    bar.draw(Rectangle{10.0F, 20.0F, 100.0F, 10.0F}, 0.5F);

    // Then the frame, background, fill and border are drawn in order.
    ASSERT_EQ(filledRects.size(), 3U);
    ASSERT_EQ(outlinedRects.size(), 1U);
    EXPECT_EQ(filledRects[0].bounds.x, 8.0F);
    EXPECT_EQ(filledRects[0].bounds.y, 18.0F);
    EXPECT_EQ(filledRects[0].bounds.width, 104.0F);
    EXPECT_EQ(filledRects[0].bounds.height, 14.0F);
    EXPECT_EQ(filledRects[1].bounds.x, 10.0F);
    EXPECT_EQ(filledRects[1].bounds.width, 100.0F);
    EXPECT_EQ(filledRects[2].bounds.width, 50.0F);
    EXPECT_EQ(outlinedRects[0].bounds.width, 100.0F);
    EXPECT_EQ(outlinedRects[0].thickness, 1.0F);
}

TEST_F(UiComponentsTests, BarClampsNegativeRatioToEmpty)
{
    // Given a bar with a negative ratio.
    const Bar bar{};

    // When it is drawn.
    bar.draw(Rectangle{0.0F, 0.0F, 100.0F, 10.0F}, -1.0F);

    // Then the fill has no width.
    ASSERT_GE(filledRects.size(), 2U);
    EXPECT_EQ(filledRects.back().bounds.width, 0.0F);
}

TEST_F(UiComponentsTests, BarClampsRatioAboveOneToFull)
{
    // Given a bar with a ratio above one.
    const Bar bar{};

    // When it is drawn.
    bar.draw(Rectangle{0.0F, 0.0F, 100.0F, 10.0F}, 2.0F);

    // Then the fill covers the whole bar.
    ASSERT_GE(filledRects.size(), 2U);
    EXPECT_EQ(filledRects.back().bounds.width, 100.0F);
}

TEST_F(UiComponentsTests, SwatchDrawsItsFillAndBorder)
{
    // Given a green swatch with a black border.
    const Swatch swatch{};

    // When it is drawn.
    swatch.draw(Rectangle{1.0F, 2.0F, 30.0F, 40.0F}, kGreen);

    // Then the fill and outline are drawn.
    ASSERT_EQ(filledRects.size(), 1U);
    ASSERT_EQ(outlinedRects.size(), 1U);
    EXPECT_EQ(filledRects[0].color.r, kGreen.r);
    EXPECT_EQ(filledRects[0].color.g, kGreen.g);
    EXPECT_EQ(filledRects[0].color.b, kGreen.b);
    EXPECT_EQ(outlinedRects[0].color.r, kBlack.r);
    EXPECT_EQ(outlinedRects[0].bounds.width, 30.0F);
}

TEST_F(UiComponentsTests, TextBlockSizesItsBackgroundFromTheWidestLine)
{
    // Given two help lines with different widths and 18px line height.
    const TextBlock block{TextBlockStyle{TextStyle{10, kWhite}, 18, 8, Color{0, 0, 0, 120}, true}};
    const std::array<std::string_view, 2> lines{"ab", "abcd"};

    // When its bounds are measured.
    const Rectangle bounds = block.bounds(Vector2{10.0F, 20.0F}, lines);

    // Then the width follows the widest line and the height follows the line count.
    EXPECT_EQ(bounds.x, 10.0F);
    EXPECT_EQ(bounds.y, 20.0F);
    EXPECT_EQ(bounds.width, 56.0F);
    EXPECT_EQ(bounds.height, 52.0F);
}

TEST_F(UiComponentsTests, TextBlockDrawsBackgroundAndEachLine)
{
    // Given two help lines.
    const TextBlock block{TextBlockStyle{TextStyle{10, kWhite}, 18, 8, Color{0, 0, 0, 120}, true}};
    const std::array<std::string_view, 2> lines{"ab", "cd"};

    // When it is drawn.
    block.draw(Vector2{10.0F, 20.0F}, lines);

    // Then the background and both lines are drawn with the configured padding.
    ASSERT_EQ(filledRects.size(), 1U);
    ASSERT_EQ(drawnTexts.size(), 2U);
    EXPECT_EQ(filledRects[0].bounds.x, 10.0F);
    EXPECT_EQ(filledRects[0].bounds.width, 36.0F);
    EXPECT_EQ(drawnTexts[0].x, 18);
    EXPECT_EQ(drawnTexts[0].y, 28);
    EXPECT_EQ(drawnTexts[1].x, 18);
    EXPECT_EQ(drawnTexts[1].y, 46);
}

TEST_F(UiComponentsTests, ButtonDrawsItsLabelCenteredInsideItsBounds)
{
    // Given a normal button.
    const Button button{ButtonStyle{}};

    // When it is drawn with a label.
    button.draw(Rectangle{0.0F, 0.0F, 100.0F, 40.0F}, "ok", ButtonState::Normal, 20);

    // Then the label is centered horizontally and vertically.
    ASSERT_EQ(drawnTexts.size(), 1U);
    EXPECT_EQ(drawnTexts[0].x, 30);
    EXPECT_EQ(drawnTexts[0].y, 10);
}

} // namespace
} // namespace view::ui
