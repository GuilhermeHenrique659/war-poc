#include "view/units/RecruitPanelView.hpp"

#include "model/player/Player.hpp"
#include "view/units/RecruitPanelLayout.hpp"
#include "view/units/UnitRenderer.hpp"
#include "view/ui/Button.hpp"
#include "view/ui/Card.hpp"
#include "view/ui/GameText.hpp"
#include "view/ui/Swatch.hpp"
#include "view/ui/Text.hpp"
#include "view/ui/TextCatalog.hpp"

#include <raylib.h>

#include <string>

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
    const Rectangle titleBounds{panel.x + recruit_panel_layout::kPadding,
        panel.y + recruit_panel_layout::kPadding + 4.0f, panel.width - 2.0f * recruit_panel_layout::kPadding,
        static_cast<float>(kTitleSize)};
    ui::Text{ui::TextStyle{kTitleSize, RAYWHITE}}.draw(titleBounds, ui::text(ui::TextId::RecruitTitle));

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
    ui::Button{closeStyle}.draw(close, ui::text(ui::TextId::CommonClose),
        hovered ? ui::ButtonState::Selected : ui::ButtonState::Normal, kLabelSize);
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
    const Rectangle swatchBounds{bounds.x + margin, bounds.y + margin, kSwatchSize, kSwatchSize};
    const bool drone = entry.kind == events::RecruitEntryKind::ConstructionDrone;
    ui::Swatch{}.draw(swatchBounds, drone ? kDroneSwatch : unitColor(entry.type));

    const float textX = swatchBounds.x + swatchBounds.width + margin;
    const float textWidth = bounds.x + bounds.width - textX;
    const float labelY = bounds.y + 8.0f;
    ui::Text{ui::TextStyle{kLabelSize, RAYWHITE}}.draw(
        Rectangle{textX, labelY, textWidth, static_cast<float>(kLabelSize)},
        drone ? ui::text(ui::TextId::DroneConstruction) : ui::unitName(entry.type));

    const float statsY = labelY + static_cast<float>(kLabelSize) + 6.0F;
    if (drone) {
        ui::Text{ui::TextStyle{kDetailSize, LIGHTGRAY}}.draw(
            Rectangle{textX, statsY, textWidth, static_cast<float>(kDetailSize)},
            ui::format(ui::TextId::RecruitDroneCapacity, entry.capacity));
        ui::Text{ui::TextStyle{kDetailSize, LIGHTGRAY}}.draw(
            Rectangle{textX, statsY + static_cast<float>(kDetailSize) + 4.0F, textWidth,
                static_cast<float>(kDetailSize)},
            ui::costText(entry.costs, entry.free));
        return;
    }

    ui::Text{ui::TextStyle{kDetailSize, LIGHTGRAY}}.draw(
        Rectangle{textX, statsY, textWidth, static_cast<float>(kDetailSize)},
        ui::format(ui::TextId::RecruitStats, entry.maxHealth, entry.damage));
    ui::Text{ui::TextStyle{kDetailSize, LIGHTGRAY}}.draw(
        Rectangle{textX, statsY + static_cast<float>(kDetailSize) + 4.0F, textWidth, static_cast<float>(kDetailSize)},
        ui::format(ui::TextId::RecruitRangeSpeed, static_cast<double>(entry.range),
            static_cast<double>(entry.speed)));
}

} // namespace view::gameplay
