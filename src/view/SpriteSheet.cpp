#include "view/SpriteSheet.hpp"

#include "view/TextureManager.hpp"

#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace view {

SpriteSheet::SpriteSheet(
    const std::string& key, const std::string& path, int frameCount, Vector2 frameSize, double framesPerSecond)
{
    load(key, path, frameCount, frameSize, framesPerSecond);
}

void SpriteSheet::load(
    const std::string& key, const std::string& path, int frameCount, Vector2 frameSize, double framesPerSecond)
{
    if (frameCount <= 0 || !std::isfinite(frameSize.x) || !std::isfinite(frameSize.y)
        || frameSize.x <= 0.0F || frameSize.y <= 0.0F) {
        throw std::invalid_argument("Sprite sheet frame count and dimensions must be positive");
    }
    if (!std::isfinite(framesPerSecond) || framesPerSecond <= 0.0) {
        throw std::invalid_argument("Sprite sheet frames per second must be positive and finite");
    }

    TextureManager& textures = TextureManager::instance();
    textures.load(key, path);
    const Texture2D texture = textures.get(key);
    const double columns = static_cast<double>(texture.width) / static_cast<double>(frameSize.x);
    const double rows = static_cast<double>(texture.height) / static_cast<double>(frameSize.y);
    if (!std::isfinite(columns) || !std::isfinite(rows) || columns < 1.0 || rows < 1.0
        || columns > static_cast<double>(std::numeric_limits<int>::max())
        || rows > static_cast<double>(std::numeric_limits<int>::max())) {
        throw std::invalid_argument("Sprite sheet frame dimensions exceed its texture");
    }
    const int columnCount = static_cast<int>(std::llround(columns));
    const int rowCount = static_cast<int>(std::llround(rows));
    constexpr double kDimensionTolerance = 0.01;
    if (std::abs(static_cast<double>(texture.width) - static_cast<double>(columnCount) * frameSize.x)
            > kDimensionTolerance
        || std::abs(static_cast<double>(texture.height) - static_cast<double>(rowCount) * frameSize.y)
            > kDimensionTolerance
        || static_cast<std::int64_t>(frameCount)
            > static_cast<std::int64_t>(columnCount) * static_cast<std::int64_t>(rowCount)) {
        throw std::invalid_argument("Sprite sheet frame dimensions and count must fit inside its texture");
    }
    SetTextureFilter(texture, TEXTURE_FILTER_POINT);

    texture_ = texture;
    frameCount_ = frameCount;
    frameSize_ = frameSize;
    columns_ = columnCount;
    rows_ = rowCount;
    framesPerSecond_ = framesPerSecond;
}

Texture2D SpriteSheet::texture() const noexcept
{
    return texture_;
}

int SpriteSheet::frameCount() const noexcept
{
    return frameCount_;
}

Vector2 SpriteSheet::frameSize() const noexcept
{
    return frameSize_;
}

int SpriteSheet::columns() const noexcept
{
    return columns_;
}

int SpriteSheet::rows() const noexcept
{
    return rows_;
}

double SpriteSheet::framesPerSecond() const noexcept
{
    return framesPerSecond_;
}

} // namespace view
