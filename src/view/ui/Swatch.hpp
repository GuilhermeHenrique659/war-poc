#pragma once

#include "view/ui/Shapes.hpp"

#include <raylib.h>

namespace view::ui {

struct SwatchStyle {
    Color border{BLACK};
    float borderWidth = 1.0F;
};

// Small colored rectangle used as a visual legend for buildings, units and items.
class Swatch {
public:
    explicit Swatch(SwatchStyle style = {})
        : style_(style)
    {
    }

    void draw(Rectangle bounds, Color color) const
    {
        fill(bounds, color);
        outline(bounds, style_.border, style_.borderWidth);
    }

private:
    SwatchStyle style_;
};

} // namespace view::ui
