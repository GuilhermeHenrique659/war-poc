#pragma once

#include <raylib.h>

#include <string>

namespace view {

class StaticSpriteViewModel {
public:
    StaticSpriteViewModel(const std::string& key, const std::string& path, Rectangle crop, int width, int height);

    void drawAt(Rectangle destination, Color tint = WHITE) const;

private:
    Texture2D texture_;
    Rectangle source_;
};

} // namespace view
