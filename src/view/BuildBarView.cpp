#include "view/BuildBarView.hpp"

#include "model/Player.hpp"
#include "view/BuildBarLayout.hpp"
#include "view/BuildingRenderer.hpp"
#include "view/IsoShapes.hpp"

#include <raylib.h>

#include <algorithm>
#include <array>

namespace view {

namespace {

constexpr Color kBarColor{24, 24, 32, 235};
constexpr Color kBarBorder{70, 70, 90, 255};
constexpr Color kSlotColor{50, 50, 62, 255};
constexpr Color kSlotSelected{120, 70, 40, 255};
constexpr Color kSlotDisabled{36, 36, 44, 255};
constexpr Color kHelpBackground{0, 0, 0, 120};
constexpr int kLabelSize = 18;
constexpr int kDetailSize = 14;
constexpr float kSwatchSize = 40.0f;

constexpr std::array kHelpLines{
    "1-9 ou clique na barra: escolher construcao",
    "Botao esquerdo: construir (arraste para muros)",
    "Botao direito / ESC: cancelar construcao",
    "Clique no Centro de Comando: recrutar tropas",
    "Clique / arraste: selecionar tropas (Shift soma)",
    "Botao direito: mover tropas selecionadas",
    "M ou botao Juntar: combinar tropas selecionadas",
    "WASD: mover jogador",
    "Zoom fixo   |   ESC: limpar selecao / menu",
};

} // namespace

BuildBarView::BuildBarView(core::EventBus& bus)
{
    bus.subscribe<events::LocalPlayerChanged>([this](const events::LocalPlayerChanged& e) { localPlayerId_ = e.player; });
    bus.subscribe<events::BuildBarChanged>([this](const events::BuildBarChanged& e) {
        if (e.player != localPlayerId_) {
            return;
        }
        entries_ = e.entries;
        selected_ = e.selected;
    });
}

void BuildBarView::draw() const
{
    const Rectangle bar = build_bar_layout::barBounds(GetScreenWidth(), GetScreenHeight());
    DrawRectangleRec(bar, kBarColor);
    DrawLineEx(Vector2{bar.x, bar.y}, Vector2{bar.x + bar.width, bar.y}, 2.0f, kBarBorder);

    for (std::size_t i = 0; i < entries_.size(); ++i) {
        drawSlot(i);
    }
    drawHelp();
}

void BuildBarView::drawSlot(std::size_t index) const
{
    const events::BuildBarEntry& entry = entries_[index];
    const Rectangle bounds = build_bar_layout::slotBounds(index, GetScreenWidth(), GetScreenHeight());
    const bool selected = selected_ == entry.type;

    const Color background = selected ? kSlotSelected : (entry.enabled ? kSlotColor : kSlotDisabled);
    DrawRectangleRec(bounds, background);
    DrawRectangleLinesEx(bounds, 2.0f, selected ? GOLD : GRAY);

    const float margin = (bounds.height - kSwatchSize) / 2.0f;
    const Rectangle swatch{bounds.x + margin, bounds.y + margin, kSwatchSize, kSwatchSize};
    const Color color = buildingColor(entry.type);
    DrawRectangleRec(swatch, entry.enabled ? color : iso::shade(color, 0.4f));
    DrawRectangleLinesEx(swatch, 1.0f, BLACK);

    const int textX = static_cast<int>(swatch.x + swatch.width + margin);
    const int labelY = static_cast<int>(bounds.y + margin);
    const int detailY = labelY + kLabelSize + 6;
    DrawText(entry.label.c_str(), textX, labelY, kLabelSize, entry.enabled ? RAYWHITE : GRAY);

    if (entry.enabled) {
        DrawText(TextFormat("%dx%d", entry.size, entry.size), textX, detailY, kDetailSize, LIGHTGRAY);
    } else {
        DrawText(entry.lockedReason.c_str(), textX, detailY, kDetailSize, Color{200, 120, 100, 255});
    }

    const char* shortcut = TextFormat("[%d]", static_cast<int>(index) + 1);
    const int shortcutX = static_cast<int>(bounds.x + bounds.width) - MeasureText(shortcut, kDetailSize) - 6;
    DrawText(shortcut, shortcutX, detailY, kDetailSize, GRAY);
}

void BuildBarView::drawHelp() const
{
    constexpr int kLineHeight = 18;
    constexpr int kPadding = 8;
    constexpr int kFontSize = 14;

    int width = 0;
    for (const char* line : kHelpLines) {
        width = std::max(width, MeasureText(line, kFontSize));
    }
    const int height = static_cast<int>(kHelpLines.size()) * kLineHeight;
    DrawRectangle(10, 10, width + 2 * kPadding, height + 2 * kPadding, kHelpBackground);

    int y = 10 + kPadding;
    for (const char* line : kHelpLines) {
        DrawText(line, 10 + kPadding, y, kFontSize, RAYWHITE);
        y += kLineHeight;
    }
}

} // namespace view
