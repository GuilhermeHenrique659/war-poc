#pragma once

#include "view/ui/Shapes.hpp"

#include <raylib.h>

namespace view::ui {

struct CardStyle {
    Color background{24, 24, 32, 235};
    Color border{70, 70, 90, 255};
    float borderWidth = 2.0F;
};

// Standard panel background used by menus, HUD and side panels.
class Card {
public:
    explicit Card(CardStyle style = {})
        : style_(style)
    {
    }

    void draw(Rectangle bounds) const
    {
        fill(bounds, style_.background);
        outline(bounds, style_.border, style_.borderWidth);
    }

private:
    CardStyle style_;
};

} // namespace view::ui
