#include "view/units/UnitRenderer.hpp"

#include "view/IsoShapes.hpp"
#include "view/ui/Bar.hpp"
#include "view/units/MechRenderer.hpp"
#include "view/units/UnitLayout.hpp"

#include <algorithm>

namespace view {

namespace {

constexpr Color kSoldierColor{190, 60, 50, 255};
constexpr Color kBatColor{75, 75, 150, 255};
constexpr Color kGoatColor{170, 145, 105, 255};
constexpr Color kCombinedColor{140, 80, 200, 255};
constexpr Color kSkinColor{230, 190, 150, 255};
constexpr Color kHornColor{60, 50, 40, 255};
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

void drawSoldier(float gx, float gy)
{
    constexpr float kBody = 16.0f;
    iso::drawPrism(centered(gx, gy, 0.3f, 0.0f, kBody), iso::boxColors(kSoldierColor, 255));
    drawHead(gx, gy, kBody);
    // Shield on the front (south-east) side.
    iso::drawPrism(iso::Prism{gx + 0.15f, gy - 0.12f, 0.06f, 0.24f, 3.0f, 11.0f}, iso::boxColors(kShieldColor, 255));
}

void drawBat(float gx, float gy)
{
    constexpr float kBody = 10.0f;
    const Color wings = iso::shade(kBatColor, 0.75f);
    const Vector2 ground = unit_layout::groundPoint(gx, gy);
    DrawTriangle(Vector2{ground.x, ground.y - 15.0f}, Vector2{ground.x - 20.0f, ground.y - 24.0f},
        Vector2{ground.x - 13.0f, ground.y - 7.0f}, wings);
    DrawTriangle(Vector2{ground.x, ground.y - 15.0f}, Vector2{ground.x + 20.0f, ground.y - 24.0f},
        Vector2{ground.x + 13.0f, ground.y - 7.0f}, wings);
    iso::drawPrism(centered(gx, gy, 0.2f, 0.0f, kBody), iso::boxColors(kBatColor, 255));
    iso::drawPrism(centered(gx, gy, 0.12f, kBody, 5.0f), iso::boxColors(kBatColor, 255));
}

void drawGoat(float gx, float gy)
{
    constexpr float kLegHeight = 7.0f;
    constexpr float kBodyHeight = 10.0f;
    const iso::BoxColors body = iso::boxColors(kGoatColor, 255);
    iso::drawPrism(iso::Prism{gx - 0.26f, gy - 0.16f, 0.52f, 0.32f, kLegHeight, kBodyHeight}, body);
    iso::drawPrism(iso::Prism{gx + 0.18f, gy - 0.12f, 0.18f, 0.24f, kLegHeight + kBodyHeight, 7.0f}, body);
    iso::drawPrism(centered(gx + 0.28f, gy - 0.12f, 0.18f, kLegHeight + kBodyHeight + 5.0f, 7.0f), body);
    const Vector2 ground = unit_layout::groundPoint(gx, gy);
    const Vector2 head = unit_layout::groundPoint(gx + 0.28f, gy - 0.12f);
    for (const float offset : {-8.0f, 8.0f}) {
        DrawLineEx(Vector2{ground.x + offset, ground.y - kLegHeight}, Vector2{ground.x + offset, ground.y},
            2.0f, kHornColor);
    }
    DrawLineEx(Vector2{head.x - 4.0f, head.y - 22.0f}, Vector2{head.x - 7.0f, head.y - 29.0f}, 2.0f, kHornColor);
    DrawLineEx(Vector2{head.x + 4.0f, head.y - 22.0f}, Vector2{head.x + 7.0f, head.y - 29.0f}, 2.0f, kHornColor);
}

} // namespace

Color unitColor(model::UnitType type)
{
    switch (type) {
    case model::UnitType::Soldier:
        return kSoldierColor;
    case model::UnitType::Bat:
        return kBatColor;
    case model::UnitType::Goat:
        return kGoatColor;
    case model::UnitType::Combined:
        return kCombinedColor;
    }
    return MAGENTA;
}

void drawUnit(model::UnitType type, float gx, float gy, int size)
{
    const Vector2 ground = unit_layout::groundPoint(gx, gy);
    const float shadow = unit_layout::halfWidth(size) * 0.85f;
    DrawEllipse(static_cast<int>(ground.x), static_cast<int>(ground.y), shadow, shadow / 2.0f, Color{0, 0, 0, 70});

    switch (type) {
    case model::UnitType::Soldier:
        drawSoldier(gx, gy);
        return;
    case model::UnitType::Bat:
        drawBat(gx, gy);
        return;
    case model::UnitType::Goat:
        drawGoat(gx, gy);
        return;
    case model::UnitType::Combined:
        drawMech(gx, gy, size);
        return;
    }
}

void drawUnitSelectionRing(float gx, float gy, int size)
{
    const Vector2 ground = unit_layout::groundPoint(gx, gy);
    const int x = static_cast<int>(ground.x);
    const int y = static_cast<int>(ground.y);
    const float radius = unit_layout::halfWidth(size) + 2.0f;
    DrawEllipseLines(x, y, radius, radius / 2.0f, kRingColor);
    DrawEllipseLines(x, y, radius + 1.0f, (radius + 1.0f) / 2.0f, kRingColor);
}

void drawUnitHealthBar(float gx, float gy, int size, float ratio)
{
    constexpr float kHeight = 4.0f;
    const float width = 26.0f * std::max(1.0f, 0.8f * static_cast<float>(size));

    const Vector2 ground = unit_layout::groundPoint(gx, gy);
    const Rectangle back{ground.x - width / 2.0f, ground.y - unit_layout::spriteHeight(size) - 6.0f, width, kHeight};
    const ui::BarStyle style{Color{20, 20, 20, 200}, kRingColor, BLACK, Color{0, 0, 0, 0}, 1.0F, 0.0F};
    ui::Bar{style}.draw(back, ratio);
}

} // namespace view
