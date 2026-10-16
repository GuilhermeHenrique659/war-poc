#include "view/PlayerRenderer.hpp"

#include "view/IsoShapes.hpp"
#include "view/UnitLayout.hpp"

#include <raylib.h>

namespace view {

namespace {

constexpr Color kSuitColor{50, 160, 220, 255};
constexpr Color kArmorColor{35, 95, 155, 255};
constexpr Color kSkinColor{230, 190, 150, 255};
constexpr Color kVisorColor{180, 240, 255, 255};

[[nodiscard]] iso::Prism centered(float gx, float gy, float size, float elevation, float height)
{
    return iso::Prism{gx - size / 2.0F, gy - size / 2.0F, size, size, elevation, height};
}

} // namespace

void drawPlayerAvatar(float gx, float gy)
{
    const Vector2 ground = unit_layout::groundPoint(gx, gy);
    DrawEllipse(static_cast<int>(ground.x), static_cast<int>(ground.y), 12.0F, 6.0F, Color{0, 0, 0, 80});

    iso::drawPrism(iso::Prism{gx - 0.16F, gy - 0.12F, 0.12F, 0.12F, 0.0F, 7.0F},
        iso::boxColors(kArmorColor, 255));
    iso::drawPrism(iso::Prism{gx + 0.04F, gy + 0.04F, 0.12F, 0.12F, 0.0F, 7.0F},
        iso::boxColors(kArmorColor, 255));
    iso::drawPrism(centered(gx, gy, 0.32F, 7.0F, 13.0F), iso::boxColors(kSuitColor, 255));
    iso::drawPrism(centered(gx, gy, 0.16F, 20.0F, 7.0F), iso::boxColors(kSkinColor, 255));

    const Vector2 visor{ground.x, ground.y - 24.0F};
    DrawLineEx(Vector2{visor.x - 3.0F, visor.y}, Vector2{visor.x + 3.0F, visor.y}, 2.0F, kVisorColor);
}

} // namespace view
