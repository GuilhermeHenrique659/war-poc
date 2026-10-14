#include "view/IsoShapes.hpp"

#include "view/IsoProjection.hpp"

namespace view::iso {

namespace {

// raylib only fills counter-clockwise triangles, so fix the winding here.
void fillTriangle(Vector2 a, Vector2 b, Vector2 c, Color color)
{
    const float cross = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    if (cross > 0.0f) {
        DrawTriangle(a, c, b, color);
        return;
    }
    DrawTriangle(a, b, c, color);
}

void fillQuad(Vector2 a, Vector2 b, Vector2 c, Vector2 d, Color color)
{
    fillTriangle(a, b, c, color);
    fillTriangle(a, c, d, color);
}

void outlineQuad(Vector2 a, Vector2 b, Vector2 c, Vector2 d, Color color)
{
    DrawLineV(a, b, color);
    DrawLineV(b, c, color);
    DrawLineV(c, d, color);
    DrawLineV(d, a, color);
}

[[nodiscard]] Vector2 raised(Vector2 point, float height)
{
    return Vector2{point.x, point.y - height};
}

[[nodiscard]] unsigned char scaleChannel(unsigned char channel, float factor)
{
    const float value = static_cast<float>(channel) * factor;
    return static_cast<unsigned char>(value > 255.0f ? 255.0f : value);
}

} // namespace

Color shade(Color color, float factor)
{
    return Color{scaleChannel(color.r, factor), scaleChannel(color.g, factor), scaleChannel(color.b, factor), color.a};
}

BoxColors boxColors(Color base, unsigned char alpha)
{
    base.a = alpha;
    return BoxColors{base, shade(base, 0.72f), shade(base, 0.55f), shade(base, 0.35f)};
}

void drawPrism(const Prism& prism, const BoxColors& colors)
{
    const Vector2 north = raised(gridToWorld(prism.x, prism.y), prism.elevation);
    const Vector2 east = raised(gridToWorld(prism.x + prism.sizeX, prism.y), prism.elevation);
    const Vector2 south = raised(gridToWorld(prism.x + prism.sizeX, prism.y + prism.sizeY), prism.elevation);
    const Vector2 west = raised(gridToWorld(prism.x, prism.y + prism.sizeY), prism.elevation);

    const Vector2 northTop = raised(north, prism.height);
    const Vector2 eastTop = raised(east, prism.height);
    const Vector2 southTop = raised(south, prism.height);
    const Vector2 westTop = raised(west, prism.height);

    fillQuad(west, south, southTop, westTop, colors.left);
    fillQuad(south, east, eastTop, southTop, colors.right);
    fillQuad(northTop, eastTop, southTop, westTop, colors.top);

    outlineQuad(northTop, eastTop, southTop, westTop, colors.outline);
    DrawLineV(west, westTop, colors.outline);
    DrawLineV(south, southTop, colors.outline);
    DrawLineV(east, eastTop, colors.outline);
    DrawLineV(west, south, colors.outline);
    DrawLineV(south, east, colors.outline);
}

void drawFootprint(float x, float y, float sizeX, float sizeY, Color fill, Color outline)
{
    const Vector2 north = gridToWorld(x, y);
    const Vector2 east = gridToWorld(x + sizeX, y);
    const Vector2 south = gridToWorld(x + sizeX, y + sizeY);
    const Vector2 west = gridToWorld(x, y + sizeY);
    fillQuad(north, east, south, west, fill);
    outlineQuad(north, east, south, west, outline);
}

} // namespace view::iso
