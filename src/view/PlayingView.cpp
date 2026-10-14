#include "view/PlayingView.hpp"

#include <raylib.h>

namespace view {

void PlayingView::draw() const
{
    constexpr const char* kMessage = "Partida em andamento";
    constexpr const char* kHint = "ESC para voltar ao menu";
    constexpr int kMessageSize = 40;
    constexpr int kHintSize = 20;

    const int centerX = GetScreenWidth() / 2;
    const int centerY = GetScreenHeight() / 2;
    DrawText(kMessage, centerX - MeasureText(kMessage, kMessageSize) / 2, centerY - kMessageSize, kMessageSize, RAYWHITE);
    DrawText(kHint, centerX - MeasureText(kHint, kHintSize) / 2, centerY + kHintSize, kHintSize, GRAY);
}

} // namespace view
