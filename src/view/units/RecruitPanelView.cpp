#include "view/units/RecruitPanelView.hpp"

#include "model/player/Player.hpp"
#include "view/units/RecruitPanelLayout.hpp"
#include "view/units/UnitRenderer.hpp"
#include "view/ui/Button.hpp"
#include "view/ui/Card.hpp"

#include <raylib.h>

namespace view::gameplay {

namespace {

constexpr Color kSlotColor{50, 50, 62, 255};
constexpr Color kSlotHover{72, 72, 90, 255};
constexpr Color kDroneSwatch{220, 160, 60, 255};
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
    ui::Card{}.draw(panel);
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
    const ui::ButtonStyle closeStyle{
        kSlotColor,
        Color{170, 60, 50, 255},
        Color{36, 36, 44, 255},
        GRAY,
        GRAY,
        RAYWHITE,
        1,
    };
    ui::Button{closeStyle}.draw(close, "X", hovered ? ui::ButtonState::Selected : ui::ButtonState::Normal, kLabelSize);
}

void RecruitPanelView::drawSlot(std::size_t index) const
{
    const events::RecruitEntry& entry = entries_[index];
    const Rectangle bounds = recruit_panel_layout::slotBounds(index, entries_.size(), GetScreenWidth());
    const bool hovered = CheckCollisionPointRec(GetMousePosition(), bounds);

    const ui::CardStyle slotStyle{
        hovered ? kSlotHover : kSlotColor,
        hovered ? GOLD : GRAY,
        2.0F,
    };
    ui::Card{slotStyle}.draw(bounds);

    const float margin = (bounds.height - kSwatchSize) / 2.0f;
    const Rectangle swatch{bounds.x + margin, bounds.y + margin, kSwatchSize, kSwatchSize};
    const bool drone = entry.kind == events::RecruitEntryKind::ConstructionDrone;
    DrawRectangleRec(swatch, drone ? kDroneSwatch : unitColor(entry.type));
    DrawRectangleLinesEx(swatch, 1.0f, BLACK);

    const int textX = static_cast<int>(swatch.x + swatch.width + margin);
    const int labelY = static_cast<int>(bounds.y + 8.0f);
    DrawText(entry.label.c_str(), textX, labelY, kLabelSize, RAYWHITE);

    const int statsY = labelY + kLabelSize + 6;
    if (drone) {
        DrawText(entry.detail.c_str(), textX, statsY, kDetailSize, LIGHTGRAY);
        DrawText(entry.cost.c_str(), textX, statsY + kDetailSize + 4, kDetailSize, LIGHTGRAY);
        return;
    }
    DrawText(TextFormat("Vida %d   Dano %d", entry.maxHealth, entry.damage), textX, statsY, kDetailSize, LIGHTGRAY);
    DrawText(TextFormat("Alcance %.0f   Vel %.1f", static_cast<double>(entry.range), static_cast<double>(entry.speed)),
        textX, statsY + kDetailSize + 4, kDetailSize, LIGHTGRAY);
}

} // namespace view::gameplay
