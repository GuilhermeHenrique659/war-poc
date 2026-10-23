#pragma once

#include <raylib.h>

#include <array>
#include <cstddef>

namespace view::settings_layout {

inline constexpr std::array<const char*, 5> kRowLabels = {
    "Resolução", "Modo de tela", "Nome do jogador", "Aplicar", "Voltar"};
inline constexpr std::size_t kRowCount = kRowLabels.size();
inline constexpr float kRowWidth = 520.0f;
inline constexpr float kRowHeight = 58.0f;
inline constexpr float kRowSpacing = 14.0f;

[[nodiscard]] inline Rectangle rowBounds(std::size_t index, int screenWidth, int screenHeight)
{
    const float total = static_cast<float>(kRowCount) * kRowHeight
        + static_cast<float>(kRowCount - 1) * kRowSpacing;
    const float x = (static_cast<float>(screenWidth) - kRowWidth) / 2.0f;
    const float y = (static_cast<float>(screenHeight) - total) / 2.0f + static_cast<float>(index) * (kRowHeight + kRowSpacing);
    return Rectangle{x, y, kRowWidth, kRowHeight};
}

} // namespace view::settings_layout
