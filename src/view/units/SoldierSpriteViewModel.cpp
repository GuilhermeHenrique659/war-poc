#include "view/units/SoldierSpriteViewModel.hpp"

#include "view/TextureManager.hpp"
#include "view/units/UnitLayout.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <utility>

namespace view {

namespace {

// The source art is not packed on an even grid and the north-east sheet is drawn larger than the
// others. Each direction is therefore cropped from the exact region that contains its frames and
// resized to the same rendered height, so all eight directions look consistent in game.
struct SoldierSheetSpec {
    const char* key;
    const char* path;
    int sourceX;
    int sourceY;
    int frameWidth;
    int frameCount;
    int sourceHeight;
};

constexpr std::array<SoldierSheetSpec, 8> kSoldierSheets{{
    {"units/soldier_n", "resource/textures/units/soldier/soldier_n.png", 197, 0, 141, 8, 141},
    {"units/soldier_ne", "resource/textures/units/soldier/soldier_ne.png", 48, 16, 164, 9, 237},
    {"units/soldier_e", "resource/textures/units/soldier/soldier_e.png", 208, 3, 140, 8, 137},
    {"units/soldier_se", "resource/textures/units/soldier/soldier_se.png", 211, 1, 139, 8, 141},
    {"units/soldier_s", "resource/textures/units/soldier/soldier_s.png", 189, 6, 142, 8, 128},
    {"units/soldier_sw", "resource/textures/units/soldier/soldier_sw.png", 187, 4, 141, 8, 150},
    {"units/soldier_w", "resource/textures/units/soldier/soldier_w.png", 183, 0, 142, 8, 137},
    {"units/soldier_nw", "resource/textures/units/soldier/soldier_nw.png", 192, 1, 141, 8, 139},
}};

constexpr float kRenderedHeight = 40.0F;
constexpr double kFramesPerSecond = 8.0;

[[nodiscard]] SpriteSheet loadSoldierSheet(const SoldierSheetSpec& spec)
{
    const Rectangle source{static_cast<float>(spec.sourceX), static_cast<float>(spec.sourceY),
        static_cast<float>(spec.frameWidth * spec.frameCount), static_cast<float>(spec.sourceHeight)};
    const int renderedFrameWidth = std::max(1,
        static_cast<int>(std::lround(
            static_cast<float>(spec.frameWidth) * kRenderedHeight / static_cast<float>(spec.sourceHeight))));
    const int renderedWidth = renderedFrameWidth * spec.frameCount;

    TextureManager::instance().loadResized(spec.key, spec.path, source, renderedWidth,
        static_cast<int>(kRenderedHeight));

    return SpriteSheet{spec.key, spec.path, spec.frameCount,
        Vector2{static_cast<float>(renderedFrameWidth), kRenderedHeight}, kFramesPerSecond};
}

[[nodiscard]] std::array<SpriteSheet, SoldierSpriteViewModel::kFacingCount> loadSoldierSheets()
{
    std::array<SpriteSheet, SoldierSpriteViewModel::kFacingCount> sheets;
    for (std::size_t index = 0; index < sheets.size(); ++index) {
        sheets[index] = loadSoldierSheet(kSoldierSheets[index]);
    }
    return sheets;
}

} // namespace

SoldierSpriteViewModel::SoldierSpriteViewModel()
    : SoldierSpriteViewModel(loadSoldierSheets())
{
}

SoldierSpriteViewModel::SoldierSpriteViewModel(std::array<SpriteSheet, kFacingCount> sheets)
{
    directions_.reserve(sheets.size());
    for (SpriteSheet& sheet : sheets) {
        const Vector2 frameSize = sheet.frameSize();
        directions_.push_back(
            DirectionAnimation{AnimatedSpriteViewModel{std::move(sheet)}, frameSize});
    }
}

void SoldierSpriteViewModel::drawAt(float gridX, float gridY, UnitFacing facing) const
{
    const auto index = static_cast<std::size_t>(facing);
    if (index >= directions_.size()) {
        throw std::out_of_range("Soldier facing has no loaded sprite sheet");
    }

    const DirectionAnimation& direction = directions_[index];
    const Vector2 ground = unit_layout::groundPoint(gridX, gridY);
    const Rectangle destination{ground.x - direction.frameSize.x / 2.0F,
        ground.y - direction.frameSize.y, direction.frameSize.x, direction.frameSize.y};
    direction.animation.drawAt(destination);
}

} // namespace view
