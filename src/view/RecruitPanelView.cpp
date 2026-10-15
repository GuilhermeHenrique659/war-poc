#include "view/RecruitPanelView.hpp"

#include "model/Player.hpp"
#include "view/RecruitPanelLayout.hpp"
#include "view/UnitRenderer.hpp"

#include <raylib.h>

namespace view {

namespace {

constexpr Color kPanelColor{24, 24, 32, 235};
constexpr Color kPanelBorder{70, 70, 90, 255};
constexpr Color kSlotColor{50, 50, 62, 255};
constexpr Color kSlotHover{72, 72, 90, 255};
constexpr int kTitleSize = 18;
constexpr int kLabelSize = 18;
constexpr int kDetailSize = 14;
constexpr float kSwatchSize = 40.0f;

} // namespace

RecruitPanelView::RecruitPanelView(core::EventBus& bus)
{
    bus.subscribe<events::LocalPlayerChanged>([this](const events::LocalPlayerChanged& e) { localPlayerId_ = e.player; });
    bus.subscribe<events::RecruitPanelChanged>([this](const events::RecruitPanelChanged& e) {
        if (e.player != localPlayerId_) {
            return;
        }
        open_ = e.open;
        entries_ = e.entries;
    });
    bus.subscribe<events::WorldReset>([this](const events::WorldReset&) {
        open_ = false;
        entries_.clear();
    });
}

void RecruitPanelView::draw() const
{
    if (!open_) {
        return;
    }

    const Rectangle panel = recruit_panel_layout::panelBounds(entries_.size(), GetScreenWidth());
    DrawRectangleRec(panel, kPanelColor);
    DrawRectangleLinesEx(panel, 2.0f, kPanelBorder);
    drawHeader();
    for (std::size_t i = 0; i < entries_.size(); ++i) {
        drawSlot(i);
    }
}

void RecruitPanelView::drawHeader() const
{
    const Rectangle panel = recruit_panel_layout::panelBounds(entries_.size(), GetScreenWidth());
    const int x = static_cast<int>(panel.x + recruit_panel_layout::kPadding);
    const int y = static_cast<int>(panel.y + recruit_panel_layout::kPadding + 4.0f);
    DrawText("Centro de Comando: recrutar", x, y, kTitleSize, RAYWHITE);

    const Rectangle close = recruit_panel_layout::closeBounds(entries_.size(), GetScreenWidth());
    const bool hovered = CheckCollisionPointRec(GetMousePosition(), close);
    DrawRectangleRec(close, hovered ? Color{170, 60, 50, 255} : kSlotColor);
    DrawRectangleLinesEx(close, 1.0f, GRAY);
    const int textX = static_cast<int>(close.x + (close.width - static_cast<float>(MeasureText("X", kLabelSize))) / 2.0f);
    const int textY = static_cast<int>(close.y + (close.height - static_cast<float>(kLabelSize)) / 2.0f);
    DrawText("X", textX, textY, kLabelSize, RAYWHITE);
}

void RecruitPanelView::drawSlot(std::size_t index) const
{
    const events::RecruitEntry& entry = entries_[index];
    const Rectangle bounds = recruit_panel_layout::slotBounds(index, entries_.size(), GetScreenWidth());
    const bool hovered = CheckCollisionPointRec(GetMousePosition(), bounds);

    DrawRectangleRec(bounds, hovered ? kSlotHover : kSlotColor);
    DrawRectangleLinesEx(bounds, 2.0f, hovered ? GOLD : GRAY);

    const float margin = (bounds.height - kSwatchSize) / 2.0f;
    const Rectangle swatch{bounds.x + margin, bounds.y + margin, kSwatchSize, kSwatchSize};
    DrawRectangleRec(swatch, unitColor(entry.type));
    DrawRectangleLinesEx(swatch, 1.0f, BLACK);

    const int textX = static_cast<int>(swatch.x + swatch.width + margin);
    const int labelY = static_cast<int>(bounds.y + 8.0f);
    DrawText(entry.label.c_str(), textX, labelY, kLabelSize, RAYWHITE);

    const int statsY = labelY + kLabelSize + 6;
    DrawText(TextFormat("Vida %d   Dano %d", entry.maxHealth, entry.damage), textX, statsY, kDetailSize, LIGHTGRAY);
    DrawText(TextFormat("Alcance %.0f   Vel %.1f", static_cast<double>(entry.range), static_cast<double>(entry.speed)),
        textX, statsY + kDetailSize + 4, kDetailSize, LIGHTGRAY);
}

} // namespace view
