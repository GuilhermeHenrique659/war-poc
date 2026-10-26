#pragma once

#include "view/ui/Shapes.hpp"
#include "view/ui/Text.hpp"

#include <raylib.h>

#include <algorithm>
#include <span>
#include <string_view>

namespace view::ui {

struct TextBlockStyle {
    TextStyle text{};
    int lineHeight = 18;
    int padding = 8;
    Color background{0, 0, 0, 120};
    bool drawBackground = true;
};

// Multi-line text with an optional backdrop, used for help and debug panels.
class TextBlock {
public:
    explicit TextBlock(TextBlockStyle style = {})
        : style_(style)
    {
    }

    [[nodiscard]] Rectangle bounds(Vector2 topLeft, std::span<const std::string_view> lines) const
    {
        const Text text{style_.text};
        int widest = 0;
        for (const std::string_view line : lines) {
            widest = std::max(widest, text.width(line));
        }
        const int height = static_cast<int>(lines.size()) * style_.lineHeight;
        return Rectangle{topLeft.x, topLeft.y, static_cast<float>(widest + 2 * style_.padding),
            static_cast<float>(height + 2 * style_.padding)};
    }

    void draw(Vector2 topLeft, std::span<const std::string_view> lines) const
    {
        const Rectangle background = bounds(topLeft, lines);
        if (style_.drawBackground) {
            fill(background, style_.background);
        }

        float y = topLeft.y + static_cast<float>(style_.padding);
        const Text text{style_.text};
        for (const std::string_view line : lines) {
            text.draw(
                Rectangle{topLeft.x + static_cast<float>(style_.padding), y, background.width, static_cast<float>(style_.lineHeight)},
                line);
            y += static_cast<float>(style_.lineHeight);
        }
    }

private:
    TextBlockStyle style_;
};

} // namespace view::ui
