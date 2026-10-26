#pragma once

#include <raylib.h>

#include <string>
#include <string_view>

namespace view::ui {

enum class TextAlign {
    Left,
    Center,
    Right,
};

enum class TextVerticalAlign {
    Top,
    Middle,
    Bottom,
};

struct TextStyle {
    int fontSize = 16;
    Color color = RAYWHITE;
    TextAlign horizontal = TextAlign::Left;
    TextVerticalAlign vertical = TextVerticalAlign::Top;
};

// Single-line text anchored inside a rectangle. raylib needs a NUL-terminated string,
// so the view converts at the call boundary.
class Text {
public:
    explicit Text(TextStyle style = {})
        : style_(style)
    {
    }

    void draw(Rectangle bounds, std::string_view value) const
    {
        const std::string label{value};
        const int textWidth = MeasureText(label.c_str(), style_.fontSize);
        float x = bounds.x;
        if (style_.horizontal == TextAlign::Center) {
            x += (bounds.width - static_cast<float>(textWidth)) / 2.0F;
        } else if (style_.horizontal == TextAlign::Right) {
            x += bounds.width - static_cast<float>(textWidth);
        }

        float y = bounds.y;
        if (style_.vertical == TextVerticalAlign::Middle) {
            y += (bounds.height - static_cast<float>(style_.fontSize)) / 2.0F;
        } else if (style_.vertical == TextVerticalAlign::Bottom) {
            y += bounds.height - static_cast<float>(style_.fontSize);
        }

        DrawText(label.c_str(), static_cast<int>(x), static_cast<int>(y), style_.fontSize, style_.color);
    }

    [[nodiscard]] int width(std::string_view value) const
    {
        const std::string label{value};
        return MeasureText(label.c_str(), style_.fontSize);
    }

    [[nodiscard]] const TextStyle& style() const { return style_; }

private:
    TextStyle style_;
};

} // namespace view::ui
