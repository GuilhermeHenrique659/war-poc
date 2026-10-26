#include "view/build/BuildBarView.hpp"

#include "model/player/Player.hpp"
#include "view/build/BuildBarLayout.hpp"
#include "view/build/BuildingRenderer.hpp"
#include "view/IsoShapes.hpp"
#include "view/ui/Card.hpp"
#include "view/ui/GameText.hpp"
#include "view/ui/Shapes.hpp"
#include "view/ui/Swatch.hpp"
#include "view/ui/Text.hpp"
#include "view/ui/TextBlock.hpp"
#include "view/ui/TextCatalog.hpp"

#include <raylib.h>

#include <array>
#include <string>
#include <string_view>

namespace view::gameplay {

namespace {

constexpr Color kBarColor{24, 24, 32, 235};
constexpr Color kBarBorder{70, 70, 90, 255};
constexpr Color kSlotColor{50, 50, 62, 255};
constexpr Color kSlotSelected{120, 70, 40, 255};
constexpr Color kSlotDisabled{36, 36, 44, 255};
constexpr Color kCreditColor{150, 215, 140, 255};
constexpr int kLabelSize = 18;
constexpr int kDetailSize = 14;
constexpr int kCostSize = 10;
constexpr float kSwatchSize = 40.0f;

constexpr std::array<ui::TextId, 13> kHelpTexts{
    ui::TextId::BuildBarHelpShortcuts,
    ui::TextId::BuildBarHelpConstruct,
    ui::TextId::BuildBarHelpCancel,
    ui::TextId::BuildBarHelpRecruit,
    ui::TextId::BuildBarHelpInventory,
    ui::TextId::BuildBarHelpSelectBuilding,
    ui::TextId::BuildBarHelpSelectUnits,
    ui::TextId::BuildBarHelpSelectUnitsAction,
    ui::TextId::BuildBarHelpMoveAttack,
    ui::TextId::BuildBarHelpSupplies,
    ui::TextId::BuildBarHelpCruzador,
    ui::TextId::BuildBarHelpMovePlayer,
    ui::TextId::BuildBarHelpZoom,
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
    const Rectangle bar = build_bar_layout::barBounds(GetScreenWidth(), GetScreenHeight(), entries_.size());
    ui::fill(bar, kBarColor);
    ui::line(Vector2{bar.x, bar.y}, Vector2{bar.x + bar.width, bar.y}, kBarBorder, 2.0f);

    for (std::size_t i = 0; i < entries_.size(); ++i) {
        drawSlot(i);
    }
    drawHelp();
}

void BuildBarView::drawSlot(std::size_t index) const
{
    const events::BuildBarEntry& entry = entries_[index];
    const Rectangle bounds
        = build_bar_layout::slotBounds(index, GetScreenWidth(), GetScreenHeight(), entries_.size());
    const bool selected = selected_ == entry.type;

    const Color background = selected ? kSlotSelected : (entry.enabled ? kSlotColor : kSlotDisabled);
    const ui::CardStyle slotStyle{background, selected ? GOLD : GRAY, 2.0F};
    ui::Card{slotStyle}.draw(bounds);

    const float margin = (bounds.height - kSwatchSize) / 2.0f;
    const Rectangle swatchBounds{bounds.x + margin, bounds.y + margin, kSwatchSize, kSwatchSize};
    const Color color = buildingColor(entry.type);
    ui::Swatch{}.draw(swatchBounds, entry.enabled ? color : iso::shade(color, 0.4f));

    const float textX = swatchBounds.x + swatchBounds.width + margin;
    const float textWidth = bounds.x + bounds.width - textX;
    const float labelY = bounds.y + 8.0f;
    const float detailY = labelY + static_cast<float>(kLabelSize) + 2.0F;
    ui::Text{ui::TextStyle{kLabelSize, entry.enabled ? RAYWHITE : GRAY}}.draw(
        Rectangle{textX, labelY, textWidth, static_cast<float>(kLabelSize)},
        ui::buildingName(entry.type));

    if (entry.enabled) {
        ui::Text{ui::TextStyle{kDetailSize, LIGHTGRAY}}.draw(
            Rectangle{textX, detailY, textWidth, static_cast<float>(kDetailSize)},
            ui::format(ui::TextId::BuildBarSize, entry.size, entry.size));
    } else {
        ui::Text{ui::TextStyle{kDetailSize, Color{200, 120, 100, 255}}}.draw(
            Rectangle{textX, detailY, textWidth, static_cast<float>(kDetailSize)},
            ui::lockReason(entry.lockReason));
    }

    ui::Text{ui::TextStyle{kCostSize, LIGHTGRAY}}.draw(
        Rectangle{textX, detailY + static_cast<float>(kDetailSize) + 2.0F, textWidth, static_cast<float>(kCostSize)},
        ui::costText(entry.costs, entry.free));

    if (!entry.free && entry.creditCost > 0.0) {
        ui::Text{ui::TextStyle{kCostSize, kCreditColor}}.draw(
            Rectangle{textX, detailY + static_cast<float>(kDetailSize) + static_cast<float>(kCostSize) + 4.0F,
                textWidth, static_cast<float>(kCostSize)},
            ui::buildCreditLabel(entry.creditCost));
    }

    if (index < build_bar_layout::kMaxShortcuts) {
        ui::Text{ui::TextStyle{kDetailSize, GRAY, ui::TextAlign::Right}}.draw(
            Rectangle{bounds.x, detailY, bounds.width - 6.0F, static_cast<float>(kDetailSize)},
            ui::format(ui::TextId::BuildBarShortcut, static_cast<int>(index) + 1));
    }
}

void BuildBarView::drawHelp() const
{
    std::array<std::string_view, kHelpTexts.size()> lines{};
    for (std::size_t i = 0; i < kHelpTexts.size(); ++i) {
        lines[i] = ui::text(kHelpTexts[i]);
    }

    const ui::TextBlock block{ui::TextBlockStyle{ui::TextStyle{14, RAYWHITE}, 18, 8, Color{0, 0, 0, 120}, true}};
    block.draw(Vector2{10.0F, 10.0F}, lines);
}

} // namespace view::gameplay
