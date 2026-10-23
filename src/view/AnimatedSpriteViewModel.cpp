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
    const Vector2 frameSize = spriteSheet_.frameSize();
    const Rectangle source{static_cast<float>(column) * frameSize.x,
        static_cast<float>(row) * frameSize.y, frameSize.x, frameSize.y};
    DrawTexturePro(spriteSheet_.texture(), source, destination, Vector2{0.0F, 0.0F}, 0.0F, tint);
}

} // namespace view
