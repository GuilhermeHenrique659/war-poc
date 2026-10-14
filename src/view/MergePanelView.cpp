#include "view/MergePanelView.hpp"

#include "model/Player.hpp"
#include "view/MergePanelLayout.hpp"

#include <raylib.h>

namespace view {

namespace {

constexpr Color kPanelColor{24, 24, 32, 235};
constexpr Color kPanelBorder{70, 70, 90, 255};
constexpr Color kButtonColor{110, 60, 160, 255};
constexpr Color kButtonHover{140, 80, 200, 255};
constexpr Color kButtonDisabled{36, 36, 44, 255};
constexpr Color kBlockedColor{200, 120, 100, 255};
constexpr int kTitleSize = 18;
constexpr int kDetailSize = 14;
constexpr int kButtonTextSize = 20;

} // namespace

MergePanelView::MergePanelView(core::EventBus& bus)
{
    bus.subscribe<events::MergePanelChanged>([this](const events::MergePanelChanged& e) {
        if (e.player == model::kLocalPlayer) {
            state_ = e;
        }
    });
    bus.subscribe<events::WorldReset>([this](const events::WorldReset&) { state_ = {}; });
}

void MergePanelView::draw() const
{
    if (!state_.open) {
        return;
    }

    const Rectangle panel = merge_panel_layout::panelBounds(GetScreenWidth());
    DrawRectangleRec(panel, kPanelColor);
    DrawRectangleLinesEx(panel, 2.0f, kPanelBorder);

    const int x = static_cast<int>(panel.x + merge_panel_layout::kPadding);
    const int y = static_cast<int>(panel.y + merge_panel_layout::kPadding + 4.0f);
    DrawText(TextFormat("%d tropas selecionadas", state_.unitCount), x, y, kTitleSize, RAYWHITE);

    drawStats();
    drawButton();
}

void MergePanelView::drawStats() const
{
    const Rectangle panel = merge_panel_layout::panelBounds(GetScreenWidth());
    const int x = static_cast<int>(panel.x + merge_panel_layout::kPadding);
    const int lineHeight = static_cast<int>(merge_panel_layout::kLineHeight);
    int y = static_cast<int>(panel.y + merge_panel_layout::kPadding + merge_panel_layout::kHeaderHeight);

    DrawText("Juntar cria uma Tropa Combinada:", x, y, kDetailSize, LIGHTGRAY);
    y += lineHeight;
    DrawText(TextFormat("Vida %d   Dano %d", state_.maxHealth, state_.damage), x, y, kDetailSize, RAYWHITE);
    y += lineHeight;
    DrawText(TextFormat("Alcance %.0f   Vel %.2f", static_cast<double>(state_.range), static_cast<double>(state_.speed)),
        x, y, kDetailSize, RAYWHITE);
    y += lineHeight;
    DrawText(TextFormat("Tamanho %dx%d", state_.size, state_.size), x, y, kDetailSize, RAYWHITE);
    y += lineHeight;
    if (!state_.canMerge) {
        DrawText(state_.blockedReason.c_str(), x, y, kDetailSize, kBlockedColor);
    }
}

void MergePanelView::drawButton() const
{
    const Rectangle button = merge_panel_layout::buttonBounds(GetScreenWidth());
    const bool hovered = CheckCollisionPointRec(GetMousePosition(), button);
    const Color color = !state_.canMerge ? kButtonDisabled : (hovered ? kButtonHover : kButtonColor);
    DrawRectangleRec(button, color);
    DrawRectangleLinesEx(button, 2.0f, state_.canMerge && hovered ? GOLD : GRAY);

    const char* label = "Juntar [M]";
    const int width = MeasureText(label, kButtonTextSize);
    const int x = static_cast<int>(button.x + (button.width - static_cast<float>(width)) / 2.0f);
    const int y = static_cast<int>(button.y + (button.height - static_cast<float>(kButtonTextSize)) / 2.0f);
    DrawText(label, x, y, kButtonTextSize, state_.canMerge ? RAYWHITE : GRAY);
}

} // namespace view
