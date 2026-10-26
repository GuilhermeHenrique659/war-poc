#pragma once

#include "view/ui/Shapes.hpp"

#include <raylib.h>

#include <algorithm>

namespace view::ui {

struct BarStyle {
    Color background{45, 45, 55, 255};
    Color fill{90, 220, 110, 255};
    Color border{BLACK};
    Color frame{0, 0, 0, 0};
    float borderWidth = 1.0F;
    float frameWidth = 0.0F;
};

// Horizontal progress bar: optional outer frame, background, ratio fill and border.
class Bar {
public:
    explicit Bar(BarStyle style = {})
        : style_(style)
    {
    }

    void draw(Rectangle bounds, float ratio) const
    {
        const float clamped = std::clamp(ratio, 0.0F, 1.0F);

        if (style_.frameWidth > 0.0F) {
            fill(Rectangle{bounds.x - style_.frameWidth, bounds.y - style_.frameWidth,
                     bounds.width + 2.0F * style_.frameWidth, bounds.height + 2.0F * style_.frameWidth},
                style_.frame);
        }

        fill(bounds, style_.background);
        fill(Rectangle{bounds.x, bounds.y, bounds.width * clamped, bounds.height}, style_.fill);

        if (style_.borderWidth > 0.0F) {
            outline(bounds, style_.border, style_.borderWidth);
        }
    }

private:
    BarStyle style_;
};

} // namespace view::ui
