#include "view/AnimatedSpriteViewModel.hpp"

#include <cmath>
#include <stdexcept>

namespace view {

AnimatedSpriteViewModel::AnimatedSpriteViewModel(SpriteSheet spriteSheet)
    : spriteSheet_(spriteSheet)
{
    const Texture2D texture = spriteSheet_.texture();
    if (texture.id == 0 || texture.width <= 0 || texture.height <= 0) {
        throw std::invalid_argument("Animated sprite requires a valid texture");
    }
    if (spriteSheet_.frameCount() <= 0 || spriteSheet_.frameSize().x <= 0.0F
        || spriteSheet_.frameSize().y <= 0.0F || spriteSheet_.columns() <= 0 || spriteSheet_.rows() <= 0) {
        throw std::invalid_argument("Animated sprite requires a loaded sprite sheet");
    }
    if (!std::isfinite(spriteSheet_.framesPerSecond()) || spriteSheet_.framesPerSecond() <= 0.0) {
        throw std::invalid_argument("Sprite sheet frames per second must be positive and finite");
    }
}

void AnimatedSpriteViewModel::drawAt(Rectangle destination, Color tint) const
{
    const double frame = std::fmod(
        std::floor(GetTime() * spriteSheet_.framesPerSecond()), static_cast<double>(spriteSheet_.frameCount()));
    const int frameIndex = static_cast<int>(frame);
    const int column = frameIndex % spriteSheet_.columns();
    const int row = frameIndex / spriteSheet_.columns();
    const Texture2D texture = spriteSheet_.texture();
    const double columnWidth = static_cast<double>(texture.width) / spriteSheet_.columns();
    const double rowHeight = static_cast<double>(texture.height) / spriteSheet_.rows();
    const int left = static_cast<int>(std::lround(static_cast<double>(column) * columnWidth));
    const int right = static_cast<int>(std::lround(static_cast<double>(column + 1) * columnWidth));
    const int top = static_cast<int>(std::lround(static_cast<double>(row) * rowHeight));
    const int bottom = static_cast<int>(std::lround(static_cast<double>(row + 1) * rowHeight));
    const Rectangle source{static_cast<float>(left), static_cast<float>(top),
        static_cast<float>(right - left), static_cast<float>(bottom - top)};
    DrawTexturePro(texture, source, destination, Vector2{0.0F, 0.0F}, 0.0F, tint);
}

} // namespace view
