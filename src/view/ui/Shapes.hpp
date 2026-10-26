#pragma once

#include <raylib.h>

namespace view::ui {

// Thin wrappers over raylib primitives so UI components do not repeat the draw calls.
inline void fill(Rectangle bounds, Color color)
{
    DrawRectangleRec(bounds, color);
}

inline void outline(Rectangle bounds, Color color, float thickness = 1.0F)
{
    DrawRectangleLinesEx(bounds, thickness, color);
}

inline void line(Vector2 from, Vector2 to, Color color, float thickness = 1.0F)
{
    DrawLineEx(from, to, thickness, color);
}

} // namespace view::ui
