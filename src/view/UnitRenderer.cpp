#include "view/UnitRenderer.hpp"

#include "view/IsoShapes.hpp"
#include "view/UnitLayout.hpp"

#include <algorithm>

namespace view {

namespace {

constexpr Color kInfantryColor{190, 60, 50, 255};
constexpr Color kRangedColor{60, 150, 70, 255};
constexpr Color kCavalryColor{210, 165, 60, 255};
constexpr Color kSkinColor{230, 190, 150, 255};
constexpr Color kHorseColor{120, 80, 50, 255};
constexpr Color kShieldColor{90, 90, 110, 255};
constexpr Color kRingColor{90, 255, 120, 255};

// Box of `size` tiles per side centered on the unit, raised by `elevation` pixels.
[[nodiscard]] iso::Prism centered(float gx, float gy, float size, float elevation, float height)
{
    return iso::Prism{gx - size / 2.0f, gy - size / 2.0f, size, size, elevation, height};
}

void drawHead(float gx, float gy, float elevation)
{
    iso::drawPrism(centered(gx, gy, 0.16f, elevation, 7.0f), iso::boxColors(kSkinColor, 255));
}

void drawInfantry(float gx, float gy)
{
    constexpr float kBody = 16.0f;
    iso::drawPrism(centered(gx, gy, 0.3f, 0.0f, kBody), iso::boxColors(kInfantryColor, 255));
    drawHead(gx, gy, kBody);
    // Shield on the front (south-east) side.
    iso::drawPrism(iso::Prism{gx + 0.15f, gy - 0.12f, 0.06f, 0.24f, 3.0f, 11.0f}, iso::boxColors(kShieldColor, 255));
}

void drawRanged(float gx, float gy)
{
    constexpr float kBody = 17.0f;
    iso::drawPrism(centered(gx, gy, 0.24f, 0.0f, kBody), iso::boxColors(kRangedColor, 255));
    drawHead(gx, gy, kBody);

    // Bow held on the side.
    const Vector2 ground = unit_layout::groundPoint(gx, gy);
    const Vector2 top{ground.x + 10.0f, ground.y - 24.0f};
    const Vector2 bottom{ground.x + 10.0f, ground.y - 4.0f};
    const Vector2 grip{ground.x + 15.0f, ground.y - 14.0f};
    DrawLineEx(top, grip, 2.0f, kHorseColor);
    DrawLineEx(grip, bottom, 2.0f, kHorseColor);
    DrawLineV(top, bottom, LIGHTGRAY);
}

void drawCavalry(float gx, float gy)
{
    constexpr float kLegs = 6.0f;
    constexpr float kHorse = 9.0f;
    constexpr float kRider = 11.0f;
    const iso::BoxColors horse = iso::boxColors(kHorseColor, 255);

    iso::drawPrism(iso::Prism{gx - 0.3f, gy - 0.14f, 0.6f, 0.28f, kLegs, kHorse}, horse);
    iso::drawPrism(iso::Prism{gx + 0.22f, gy - 0.1f, 0.16f, 0.2f, kLegs + kHorse, 7.0f}, horse);
    iso::drawPrism(centered(gx, gy, 0.22f, kLegs + kHorse, kRider), iso::boxColors(kCavalryColor, 255));
    drawHead(gx, gy, kLegs + kHorse + kRider);

    const Vector2 ground = unit_layout::groundPoint(gx, gy);
    const Color legs = iso::shade(kHorseColor, 0.6f);
    DrawLineEx(Vector2{ground.x - 10.0f, ground.y - kLegs}, Vector2{ground.x - 10.0f, ground.y}, 2.0f, legs);
    DrawLineEx(Vector2{ground.x + 10.0f, ground.y - kLegs}, Vector2{ground.x + 10.0f, ground.y}, 2.0f, legs);
}

} // namespace

Color unitColor(model::UnitType type)
{
    switch (type) {
    case model::UnitType::Infantry:
        return kInfantryColor;
    case model::UnitType::Ranged:
        return kRangedColor;
    case model::UnitType::Cavalry:
        return kCavalryColor;
    }
    return MAGENTA;
}

void drawUnit(model::UnitType type, float gx, float gy)
{
    const Vector2 ground = unit_layout::groundPoint(gx, gy);
    DrawEllipse(static_cast<int>(ground.x), static_cast<int>(ground.y), 11.0f, 5.5f, Color{0, 0, 0, 70});

    switch (type) {
    case model::UnitType::Infantry:
        drawInfantry(gx, gy);
        return;
    case model::UnitType::Ranged:
        drawRanged(gx, gy);
        return;
    case model::UnitType::Cavalry:
        drawCavalry(gx, gy);
        return;
    }
}

void drawUnitSelectionRing(float gx, float gy)
{
    const Vector2 ground = unit_layout::groundPoint(gx, gy);
    const int x = static_cast<int>(ground.x);
    const int y = static_cast<int>(ground.y);
    DrawEllipseLines(x, y, 15.0f, 7.5f, kRingColor);
    DrawEllipseLines(x, y, 16.0f, 8.0f, kRingColor);
}

void drawUnitHealthBar(float gx, float gy, float ratio)
{
    constexpr float kWidth = 26.0f;
    constexpr float kHeight = 4.0f;

    const Vector2 ground = unit_layout::groundPoint(gx, gy);
    const Rectangle back{ground.x - kWidth / 2.0f, ground.y - unit_layout::kHitHeight - 6.0f, kWidth, kHeight};
    DrawRectangleRec(back, Color{20, 20, 20, 200});
    DrawRectangleRec(Rectangle{back.x, back.y, kWidth * std::clamp(ratio, 0.0f, 1.0f), kHeight}, kRingColor);
    DrawRectangleLinesEx(back, 1.0f, BLACK);
}

} // namespace view
