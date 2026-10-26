#include "view/build/BuildingRenderer.hpp"

#include "view/IsoProjection.hpp"
#include "view/IsoShapes.hpp"
#include "view/ui/Bar.hpp"

#include <algorithm>

namespace view {

namespace {

constexpr Color kCommandCenterColor{70, 120, 200, 255};
constexpr Color kWallColor{150, 148, 138, 255};
constexpr Color kFlagColor{220, 60, 50, 255};
constexpr Color kEconomyColor{90, 160, 100, 255};
constexpr Color kBarracksColor{160, 100, 75, 255};
constexpr Color kCruzadorColor{100, 95, 180, 255};
constexpr Color kTowerColor{190, 125, 55, 255};
constexpr Color kMineColor{105, 105, 110, 255};
constexpr Color kSteelworksColor{125, 135, 145, 255};
constexpr Color kFactoryColor{135, 115, 155, 255};
constexpr Color kWeaponFactoryColor{150, 90, 80, 255};
constexpr Color kAmmoFactoryColor{150, 130, 70, 255};
constexpr Color kTrainingDummyColor{175, 145, 90, 255};

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

void drawTrainingDummy(model::GridPos origin, unsigned char alpha)
{
    constexpr float kPostHeight = 34.0f;
    constexpr float kPostSize = 0.16f;
    constexpr float kArmHeight = 3.0f;
    constexpr float kArmLength = 0.8f;

    const float x = static_cast<float>(origin.x);
    const float y = static_cast<float>(origin.y);
    const float postOffset = (1.0f - kPostSize) / 2.0f;
    const float armOffset = (1.0f - kArmLength) / 2.0f;

    const iso::BoxColors wood = iso::boxColors(kTrainingDummyColor, alpha);
    const iso::BoxColors straw = iso::boxColors(iso::shade(kTrainingDummyColor, 1.15f), alpha);

    iso::drawPrism({x + postOffset, y + postOffset, kPostSize, kPostSize, 0.0f, kPostHeight}, wood);
    iso::drawPrism({x + armOffset, y + 0.45f, kArmLength, 0.1f, 24.0f, kArmHeight}, wood);
    iso::drawPrism({x + 0.45f, y + armOffset, 0.1f, kArmLength, 24.0f, kArmHeight}, wood);
    iso::drawPrism({x + 0.34f, y + 0.34f, 0.32f, 0.32f, 12.0f, 14.0f}, straw);
    iso::drawPrism({x + 0.38f, y + 0.38f, 0.24f, 0.24f, 30.0f, 8.0f}, straw);
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
    case model::BuildingType::House:
    case model::BuildingType::Generator:
    case model::BuildingType::Warehouse:
        return kEconomyColor;
    case model::BuildingType::Barracks:
        return kBarracksColor;
    case model::BuildingType::Cruzador:
        return kCruzadorColor;
    case model::BuildingType::Tower:
        return kTowerColor;
    case model::BuildingType::IronMine:
    case model::BuildingType::CoalMine:
    case model::BuildingType::CopperMine:
        return kMineColor;
    case model::BuildingType::Steelworks:
        return kSteelworksColor;
    case model::BuildingType::ElectronicsFactory:
    case model::BuildingType::ToolFactory:
        return kFactoryColor;
    case model::BuildingType::WeaponFactory:
        return kWeaponFactoryColor;
    case model::BuildingType::AmmoFactory:
        return kAmmoFactoryColor;
    case model::BuildingType::TrainingDummy:
        return kTrainingDummyColor;
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
    case model::BuildingType::TrainingDummy:
        drawTrainingDummy(origin, alpha);
        return;
    case model::BuildingType::House:
    case model::BuildingType::Generator:
    case model::BuildingType::Warehouse:
    case model::BuildingType::Barracks:
    case model::BuildingType::Cruzador:
    case model::BuildingType::IronMine:
    case model::BuildingType::CoalMine:
    case model::BuildingType::Steelworks:
    case model::BuildingType::CopperMine:
    case model::BuildingType::ElectronicsFactory:
    case model::BuildingType::ToolFactory:
    case model::BuildingType::WeaponFactory:
    case model::BuildingType::AmmoFactory:
        iso::drawPrism({static_cast<float>(origin.x), static_cast<float>(origin.y), static_cast<float>(size),
                           static_cast<float>(size), 0.0f, 24.0f},
            iso::boxColors(buildingColor(type), alpha));
        return;
    case model::BuildingType::Tower:
        iso::drawPrism({static_cast<float>(origin.x), static_cast<float>(origin.y), static_cast<float>(size),
                           static_cast<float>(size), 0.0f, 36.0f},
            iso::boxColors(buildingColor(type), alpha));
        return;
    }
}

void drawConstructionProgressBar(model::GridPos origin, int size, float progress)
{
    constexpr float kBarWidth = 96.0F;
    constexpr float kBarHeight = 10.0F;
    const float centerX = static_cast<float>(origin.x) + static_cast<float>(size) / 2.0F;
    const float centerY = static_cast<float>(origin.y) + static_cast<float>(size) / 2.0F;
    const Vector2 center = iso::gridToWorld(centerX, centerY);
    const float left = center.x - kBarWidth / 2.0F;
    const float top = center.y - 40.0F - static_cast<float>(size) * 12.0F;
    const float ratio = std::clamp(progress, 0.0F, 1.0F);
    const ui::BarStyle style{Color{45, 45, 55, 255}, Color{230, 190, 70, 255}, Color{0, 0, 0, 0},
        BLACK, 0.0F, 2.0F};
    ui::Bar{style}.draw(Rectangle{left, top, kBarWidth, kBarHeight}, ratio);
}

} // namespace view
