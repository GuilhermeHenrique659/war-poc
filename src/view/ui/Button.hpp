#pragma once

#include "view/ui/Shapes.hpp"
#include "view/ui/Text.hpp"

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

        fill(bounds, background);
        outline(bounds, border, static_cast<float>(style_.borderWidth));
        Text{TextStyle{fontSize, text, TextAlign::Center, TextVerticalAlign::Middle}}.draw(bounds, label);
    }

private:
    ButtonStyle style_;
};

} // namespace view::ui
