#pragma once

#include <raylib.h>

namespace view {

class SpriteAnimation {
public:
    SpriteAnimation(Texture2D texture, int columns, int rows, double frameDuration);

    void drawAt(Rectangle destination) const;

private:
    Texture2D texture_;
    int columns_;
    int rows_;
    double frameDuration_;
};

} // namespace view
