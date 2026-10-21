#include "view/units/MechRenderer.hpp"

#include "view/IsoShapes.hpp"
#include "view/units/UnitLayout.hpp"

#include <raylib.h>

namespace view {

namespace {

constexpr Color kArmorColor{140, 80, 200, 255};
constexpr Color kMetalColor{150, 155, 170, 255};
constexpr Color kDarkMetalColor{80, 82, 95, 255};
constexpr Color kGlowColor{90, 230, 255, 255};

// Every measure below is for a 1x1 mech and gets multiplied by the size.
struct MechFrame {
    float gx;
    float gy;
    float scale;

    // Box of `size` tiles per side centered on the mech, shifted sideways on screen
    // (positive = right) and raised by `elevation` pixels.
    [[nodiscard]] iso::Prism box(float size, float lateral, float elevation, float height) const
    {
        const float s = size * scale;
        const float a = lateral * scale;
        return iso::Prism{gx + a - s / 2.0f, gy - a - s / 2.0f, s, s, elevation * scale, height * scale};
    }

    // Screen offset from the ground point, in pixels scaled with the mech.
    [[nodiscard]] Vector2 point(Vector2 ground, float dx, float dy) const
    {
        return Vector2{ground.x + dx * scale, ground.y + dy * scale};
    }
};

void drawPair(const MechFrame& mech, float size, float lateral, float elevation, float height, Color color)
{
    const iso::BoxColors colors = iso::boxColors(color, 255);
    iso::drawPrism(mech.box(size, -lateral, elevation, height), colors);
    iso::drawPrism(mech.box(size, lateral, elevation, height), colors);
}

} // namespace

void drawMech(float gx, float gy, int size)
{
    const MechFrame mech{gx, gy, static_cast<float>(size)};
    const Vector2 ground = unit_layout::groundPoint(gx, gy);

    drawPair(mech, 0.26f, 0.17f, 0.0f, 3.0f, kDarkMetalColor); // Feet.
    drawPair(mech, 0.18f, 0.17f, 3.0f, 8.0f, kMetalColor); // Legs.
    iso::drawPrism(mech.box(0.36f, 0.0f, 11.0f, 3.0f), iso::boxColors(kDarkMetalColor, 255)); // Hip.
    iso::drawPrism(mech.box(0.5f, 0.0f, 14.0f, 11.0f), iso::boxColors(kArmorColor, 255)); // Torso.
    drawPair(mech, 0.13f, 0.3f, 12.0f, 9.0f, kMetalColor); // Arms.
    drawPair(mech, 0.2f, 0.3f, 21.0f, 5.0f, iso::shade(kArmorColor, 1.2f)); // Shoulders.
    iso::drawPrism(mech.box(0.22f, 0.0f, 25.0f, 6.0f), iso::boxColors(kMetalColor, 255)); // Head.

    // Chest core and visor sit on the front edge of the torso and head.
    const Vector2 core = mech.point(ground, 0.0f, -11.5f);
    DrawCircleV(core, 2.2f * mech.scale, kGlowColor);
    DrawLineEx(mech.point(ground, -3.0f, -24.5f), mech.point(ground, 3.0f, -24.5f), 1.5f * mech.scale, kGlowColor);

    // Shoulder cannon.
    DrawLineEx(mech.point(ground, 19.0f, -26.0f), mech.point(ground, 25.0f, -34.0f), 2.5f * mech.scale, kDarkMetalColor);
}

} // namespace view
