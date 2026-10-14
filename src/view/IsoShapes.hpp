#pragma once

#include <raylib.h>

// Placeholder isometric primitives, drawn in world space (inside BeginMode2D).
namespace view::iso {

struct BoxColors {
    Color top;
    Color left;
    Color right;
    Color outline;
};

// Axis-aligned box on the grid. Position and size are in tiles, elevation and height in pixels.
struct Prism {
    float x;
    float y;
    float sizeX;
    float sizeY;
    float elevation;
    float height;
};

[[nodiscard]] Color shade(Color color, float factor);
[[nodiscard]] BoxColors boxColors(Color base, unsigned char alpha);

void drawPrism(const Prism& prism, const BoxColors& colors);
void drawFootprint(float x, float y, float sizeX, float sizeY, Color fill, Color outline);

} // namespace view::iso
