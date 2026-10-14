#include "view/BuildingRenderer.hpp"

#include "view/IsoProjection.hpp"
#include "view/IsoShapes.hpp"

namespace view {

namespace {

constexpr Color kCommandCenterColor{70, 120, 200, 255};
constexpr Color kWallColor{150, 148, 138, 255};
constexpr Color kFlagColor{220, 60, 50, 255};

void drawCommandCenter(model::GridPos origin, int size, unsigned char alpha)
{
    constexpr float kBaseHeight = 26.0f;
    constexpr float kTurretHeight = 22.0f;
    constexpr float kTowerHeight = 40.0f;
    constexpr float kTowerSize = 2.0f;
    constexpr float kPoleHeight = 26.0f;

    const float x = static_cast<float>(origin.x);
    const float y = static_cast<float>(origin.y);
    const float s = static_cast<float>(size);
    const float far = s - 1.0f;
    const float towerOffset = (s - kTowerSize) / 2.0f;

    const iso::BoxColors base = iso::boxColors(kCommandCenterColor, alpha);
    const iso::BoxColors turret = iso::boxColors(iso::shade(kCommandCenterColor, 1.2f), alpha);

    // Back to front so nearer pieces overlap farther ones.
    iso::drawPrism({x, y, s, s, 0.0f, kBaseHeight}, base);
    iso::drawPrism({x, y, 1.0f, 1.0f, kBaseHeight, kTurretHeight}, turret);
    iso::drawPrism({x + towerOffset, y + towerOffset, kTowerSize, kTowerSize, kBaseHeight, kTowerHeight}, turret);
    iso::drawPrism({x, y + far, 1.0f, 1.0f, kBaseHeight, kTurretHeight}, turret);
    iso::drawPrism({x + far, y, 1.0f, 1.0f, kBaseHeight, kTurretHeight}, turret);
    iso::drawPrism({x + far, y + far, 1.0f, 1.0f, kBaseHeight, kTurretHeight}, turret);

    const float center = towerOffset + kTowerSize / 2.0f;
    const Vector2 groundCenter = iso::gridToWorld(x + center, y + center);
    const Vector2 poleBase{groundCenter.x, groundCenter.y - kBaseHeight - kTowerHeight};
    const Vector2 poleTop{poleBase.x, poleBase.y - kPoleHeight};
    Color flag = kFlagColor;
    flag.a = alpha;
    DrawLineEx(poleBase, poleTop, 2.0f, Color{40, 40, 40, alpha});
    DrawTriangle(poleTop, Vector2{poleTop.x, poleTop.y + 10.0f}, Vector2{poleTop.x + 16.0f, poleTop.y + 5.0f}, flag);
}

void drawWall(model::GridPos origin, unsigned char alpha)
{
    constexpr float kHeight = 22.0f;
    constexpr float kMerlonSize = 0.4f;
    constexpr float kMerlonHeight = 7.0f;

    const float x = static_cast<float>(origin.x);
    const float y = static_cast<float>(origin.y);
    const float merlonOffset = (1.0f - kMerlonSize) / 2.0f;

    iso::drawPrism({x, y, 1.0f, 1.0f, 0.0f, kHeight}, iso::boxColors(kWallColor, alpha));
    iso::drawPrism({x + merlonOffset, y + merlonOffset, kMerlonSize, kMerlonSize, kHeight, kMerlonHeight},
        iso::boxColors(iso::shade(kWallColor, 1.1f), alpha));
}

} // namespace

Color buildingColor(model::BuildingType type)
{
    switch (type) {
    case model::BuildingType::CommandCenter:
        return kCommandCenterColor;
    case model::BuildingType::Wall:
        return kWallColor;
    }
    return MAGENTA;
}

void drawBuilding(model::BuildingType type, model::GridPos origin, int size, unsigned char alpha)
{
    switch (type) {
    case model::BuildingType::CommandCenter:
        drawCommandCenter(origin, size, alpha);
        return;
    case model::BuildingType::Wall:
        drawWall(origin, alpha);
        return;
    }
}

} // namespace view
