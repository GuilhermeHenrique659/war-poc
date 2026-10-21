#pragma once

#include <raylib.h>

#include <string>

namespace view::ui {

enum class ButtonState {
    Normal,
    Selected,
    Disabled,
};

struct ButtonStyle {
    Color background{50, 50, 60, 255};
    Color selectedBackground{180, 60, 50, 255};
    Color disabledBackground{36, 36, 44, 255};
    Color border{128, 128, 128, 255};
    Color selectedBorder{255, 203, 0, 255};
    Color text{245, 245, 245, 255};
    int borderWidth = 2;
};

class Button {
public:
    explicit Button(ButtonStyle style = {})
        : style_(style)
    {
    }

    void draw(Rectangle bounds, const std::string& label, ButtonState state, int fontSize) const
    {
        const bool selected = state == ButtonState::Selected;
        const Color background = state == ButtonState::Disabled
            ? style_.disabledBackground
            : selected ? style_.selectedBackground : style_.background;
        const Color border = selected ? style_.selectedBorder : style_.border;
        const Color text = state == ButtonState::Disabled ? Color{128, 128, 128, 255} : style_.text;

        DrawRectangleRec(bounds, background);
        DrawRectangleLinesEx(bounds, static_cast<float>(style_.borderWidth), border);

        const int textWidth = MeasureText(label.c_str(), fontSize);
        const int x = static_cast<int>(bounds.x + (bounds.width - static_cast<float>(textWidth)) / 2.0F);
        const int y = static_cast<int>(bounds.y + (bounds.height - static_cast<float>(fontSize)) / 2.0F);
        DrawText(label.c_str(), x, y, fontSize, text);
    }

private:
    ButtonStyle style_;
};

} // namespace view::ui
