#pragma once

#include <raylib.h>

namespace view::ui {

struct CardStyle {
    Color background{24, 24, 32, 235};
    Color border{70, 70, 90, 255};
    float borderWidth = 2.0F;
};

class Card {
public:
    explicit Card(CardStyle style = {})
        : style_(style)
    {
    }

    void draw(Rectangle bounds) const
    {
        DrawRectangleRec(bounds, style_.background);
        DrawRectangleLinesEx(bounds, style_.borderWidth, style_.border);
    }

private:
    CardStyle style_;
};

} // namespace view::ui
