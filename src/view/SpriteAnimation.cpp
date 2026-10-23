#include "view/SpriteAnimation.hpp"

#include <cmath>
#include <stdexcept>

namespace view {

SpriteAnimation::SpriteAnimation(Texture2D texture, int columns, int rows, double frameDuration)
    : texture_(texture)
    , columns_(columns)
    , rows_(rows)
    , frameDuration_(frameDuration)
{
    if (texture_.id == 0 || texture_.width <= 0 || texture_.height <= 0) {
        throw std::invalid_argument("Sprite animation requires a valid texture");
    }
    if (columns_ <= 0 || rows_ <= 0 || columns_ > texture_.width || rows_ > texture_.height) {
        throw std::invalid_argument("Sprite animation grid must fit inside the texture");
    }
    if (!std::isfinite(frameDuration_) || frameDuration_ <= 0.0) {
        throw std::invalid_argument("Sprite animation frame duration must be positive and finite");
    }
}

void SpriteAnimation::drawAt(Rectangle destination) const
{
    const double totalFrames = static_cast<double>(columns_) * static_cast<double>(rows_);
    double frame = std::fmod(std::floor(GetTime() / frameDuration_), totalFrames);
    if (frame < 0.0) {
        frame += totalFrames;
    }

    const int frameIndex = static_cast<int>(frame);
    const int column = frameIndex % columns_;
    const int row = frameIndex / columns_;
    const float frameWidth = static_cast<float>(texture_.width) / static_cast<float>(columns_);
    const float frameHeight = static_cast<float>(texture_.height) / static_cast<float>(rows_);
    const Rectangle source{static_cast<float>(column) * frameWidth, static_cast<float>(row) * frameHeight,
        frameWidth, frameHeight};
    DrawTexturePro(texture_, source, destination, Vector2{0.0F, 0.0F}, 0.0F, WHITE);
}

} // namespace view
