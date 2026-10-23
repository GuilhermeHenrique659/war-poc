#pragma once

#include "view/SpriteSheet.hpp"

namespace view {

class AnimatedSpriteViewModel {
public:
    explicit AnimatedSpriteViewModel(SpriteSheet spriteSheet);

    void drawAt(Rectangle destination, Color tint = WHITE) const;

private:
    SpriteSheet spriteSheet_;
};

} // namespace view
