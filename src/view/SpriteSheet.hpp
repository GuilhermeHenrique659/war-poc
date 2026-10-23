#pragma once

#include <raylib.h>

#include <string>

namespace view {

class SpriteSheet {
public:
    SpriteSheet() = default;
    SpriteSheet(const std::string& key, const std::string& path, int frameCount, Vector2 frameSize,
        double framesPerSecond);

    void load(const std::string& key, const std::string& path, int frameCount, Vector2 frameSize,
        double framesPerSecond);

    [[nodiscard]] Texture2D texture() const noexcept;
    [[nodiscard]] int frameCount() const noexcept;
    [[nodiscard]] Vector2 frameSize() const noexcept;
    [[nodiscard]] int columns() const noexcept;
    [[nodiscard]] int rows() const noexcept;
    [[nodiscard]] double framesPerSecond() const noexcept;

private:
    Texture2D texture_{};
    int frameCount_ = 0;
    Vector2 frameSize_{};
    int columns_ = 0;
    int rows_ = 0;
    double framesPerSecond_ = 0.0;
};

} // namespace view
