#include "view/economy/InventoryPanelView.hpp"

#include "model/economy/Item.hpp"
#include "view/economy/InventoryPanelLayout.hpp"
#include "view/ui/Button.hpp"
#include "view/ui/Card.hpp"
#include "view/ui/GameText.hpp"
#include "view/ui/Shapes.hpp"
#include "view/ui/Text.hpp"
#include "view/ui/TextCatalog.hpp"

#include <raylib.h>

#include <algorithm>
#include <string>
#include <vector>

namespace view::gameplay {
namespace {

constexpr Color kSlotColor{50, 50, 62, 255};
constexpr Color kSlotHover{72, 105, 78, 255};
constexpr int kTitleSize = 18;
constexpr int kTextSize = 14;
constexpr int kSlotTextSize = 10;
constexpr int kUnitNameSize = 15;
constexpr int kUnitDetailSize = 12;
constexpr int kUnitSlotTextSize = 8;

[[nodiscard]] int itemAmount(const std::vector<events::InventorySlotState>& slots, model::ItemId item)
{
    int amount = 0;
    for (const auto& slot : slots) {
        if (slot.occupied && slot.item == item) {
            amount += slot.currentQuantity;
        }
    }
    return amount;
}

[[nodiscard]] std::string paneTitle(const events::InventoryPaneState& pane)
{
    switch (pane.role) {
    case events::InventoryPaneRole::Player:
        return ui::text(ui::TextId::InventoryPlayer);
    case events::InventoryPaneRole::Units:
        return ui::text(ui::TextId::InventoryUnits);
    case events::InventoryPaneRole::TransferTarget:
        return pane.buildingType ? std::string{ui::buildingName(*pane.buildingType)}
                                 : std::string{ui::text(ui::TextId::CommonUnknown)};
    }
    return std::string{ui::text(ui::TextId::CommonUnknown)};
}

} // namespace

InventoryPanelView::InventoryPanelView(core::EventBus& bus)
{
    bus.subscribe<events::LocalPlayerChanged>([this](const events::LocalPlayerChanged& event) {
        localPlayerId_ = event.player;
        panel_ = {};
        drag_ = {};
    });
    bus.subscribe<events::InventoryPanelChanged>([this](const events::InventoryPanelChanged& event) {
        if (event.player == localPlayerId_) {
            panel_ = event;
        }
    });
    bus.subscribe<events::InventoryDragChanged>([this](const events::InventoryDragChanged& event) {
        if (event.player == localPlayerId_) {
            drag_ = event;
        }
    });
    bus.subscribe<events::WorldReset>([this](const events::WorldReset&) {
        panel_ = {};
        drag_ = {};
    });
}

void InventoryPanelView::draw() const
{
    if (!panel_.open) {
        return;
    }

    const Rectangle panelBounds = inventory_panel_layout::panelBounds(GetScreenWidth());
    ui::Card{}.draw(panelBounds);
    const Rectangle titleBounds{panelBounds.x + inventory_panel_layout::kPadding,
        panelBounds.y + inventory_panel_layout::kPadding, panelBounds.width - 2.0F * inventory_panel_layout::kPadding,
        static_cast<float>(kTitleSize)};
    ui::Text{ui::TextStyle{kTitleSize, RAYWHITE}}.draw(titleBounds, ui::text(ui::TextId::InventoryTitle));

    const Rectangle closeBounds = inventory_panel_layout::closeBounds(GetScreenWidth());
    const bool closeHovered = CheckCollisionPointRec(GetMousePosition(), closeBounds);
    const ui::ButtonStyle closeStyle{
        kSlotColor, Color{170, 60, 50, 255}, Color{36, 36, 44, 255}, GRAY, GRAY, RAYWHITE, 1};
    ui::Button{closeStyle}.draw(
        closeBounds, ui::text(ui::TextId::CommonClose),
        closeHovered ? ui::ButtonState::Selected : ui::ButtonState::Normal, kTextSize);

    for (std::size_t index = 0; index < panel_.panes.size(); ++index) {
        drawPane(panel_.panes[index], index);
    }

    if (drag_.active) {
        const Vector2 mouse = GetMousePosition();
        const Rectangle cursor{mouse.x + 12.0F, mouse.y + 12.0F, 105.0F, 36.0F};
        ui::fill(cursor, Color{20, 20, 28, 230});
        ui::outline(cursor, GOLD, 1.0F);
        ui::Text{ui::TextStyle{kTextSize, RAYWHITE}}.draw(
            Rectangle{cursor.x + 5.0F, cursor.y + 10.0F, cursor.width, static_cast<float>(kTextSize)},
            ui::format(ui::TextId::InventoryItemQuantity, ui::itemName(drag_.item), drag_.quantity));
    }
}

void InventoryPanelView::drawPane(
    const events::InventoryPaneState& pane, std::size_t paneIndex) const
{
    if (pane.layout == events::InventoryPaneLayout::Rows) {
        drawUnitsPane(pane, paneIndex);
        return;
    }
    drawSlotsPane(pane, paneIndex);
}

void InventoryPanelView::drawSlotsPane(
    const events::InventoryPaneState& pane, std::size_t paneIndex) const
{
    const int screenWidth = GetScreenWidth();
    const int paneX = static_cast<int>(inventory_panel_layout::paneBounds(paneIndex, screenWidth).x);
    const int paneY = static_cast<int>(inventory_panel_layout::panelBounds(screenWidth).y + 44.0F);
    const auto usedSlots = static_cast<int>(std::count_if(
        pane.slots.begin(), pane.slots.end(), [](const auto& slot) { return slot.occupied; }));
    const ui::Text text{ui::TextStyle{kTextSize, RAYWHITE}};
    text.draw(Rectangle{static_cast<float>(paneX), static_cast<float>(paneY), 260.0F, static_cast<float>(kTextSize)},
        ui::format(ui::TextId::InventoryPaneCount, paneTitle(pane), usedSlots,
            static_cast<int>(pane.slots.size())));

    if (pane.showSummary) {
        const ui::Text summaryText{ui::TextStyle{kTextSize, LIGHTGRAY}};
        for (std::size_t index = 0; index < model::kAllItems.size(); ++index) {
            const model::ItemId item = model::kAllItems[index];
            const std::size_t column = index % 2;
            const std::size_t row = index / 2;
            const int summaryX = paneX + static_cast<int>(column) * 144;
            summaryText.draw(Rectangle{static_cast<float>(summaryX),
                    static_cast<float>(paneY + 20 + static_cast<int>(row) * 18), 140.0F,
                    static_cast<float>(kTextSize)},
                ui::format(ui::TextId::InventoryItemQuantity, ui::itemName(item), itemAmount(pane.slots, item)));
        }
    }
    if (pane.showTransferHint) {
        ui::Text{ui::TextStyle{kTextSize, LIGHTGRAY}}.draw(
            Rectangle{static_cast<float>(paneX),
                static_cast<float>(paneY + (pane.showSummary ? 96 : 62)), 260.0F, static_cast<float>(kTextSize)},
            ui::text(ui::TextId::InventoryTransferHint));
    }

    for (std::size_t index = 0; index < pane.slots.size(); ++index) {
        drawSlot(pane, paneIndex, index);
    }
}

void InventoryPanelView::drawSlot(
    const events::InventoryPaneState& pane, std::size_t paneIndex, std::size_t index) const
{
    const Rectangle bounds
        = inventory_panel_layout::slotBounds(index, paneIndex, GetScreenWidth());
    const bool hovered = CheckCollisionPointRec(GetMousePosition(), bounds);
    const bool occupied = pane.slots[index].occupied;
    const ui::CardStyle style{
        occupied && hovered ? kSlotHover : kSlotColor,
        hovered ? GOLD : GRAY,
        1.0F,
    };
    ui::Card{style}.draw(bounds);
    if (!occupied) {
        return;
    }

    const auto& slot = pane.slots[index];
    ui::Text{ui::TextStyle{kSlotTextSize, RAYWHITE}}.draw(
        Rectangle{bounds.x + 2.0F, bounds.y + 4.0F, bounds.width - 4.0F, static_cast<float>(kSlotTextSize)},
        ui::itemName(slot.item));
    ui::Text{ui::TextStyle{kSlotTextSize, LIGHTGRAY}}.draw(
        Rectangle{bounds.x + 2.0F, bounds.y + 17.0F, bounds.width - 4.0F, static_cast<float>(kSlotTextSize)},
        std::to_string(slot.currentQuantity));
}

void InventoryPanelView::drawUnitsPane(
    const events::InventoryPaneState& pane, std::size_t paneIndex) const
{
    const int screenWidth = GetScreenWidth();
    const int paneX = static_cast<int>(inventory_panel_layout::paneBounds(paneIndex, screenWidth).x);
    const int paneY = static_cast<int>(inventory_panel_layout::panelBounds(screenWidth).y + 44.0F);

    const ui::Text title{ui::TextStyle{kTextSize, RAYWHITE}};
    title.draw(Rectangle{static_cast<float>(paneX), static_cast<float>(paneY), 260.0F, static_cast<float>(kTextSize)},
        ui::format(ui::TextId::InventoryUnitsCount, paneTitle(pane), static_cast<int>(pane.units.size())));

    const std::size_t rows
        = std::min(pane.units.size(), inventory_panel_layout::kMaxUnitRows);
    const ui::Text hint{ui::TextStyle{kTextSize, LIGHTGRAY}};
    if (pane.units.size() > rows) {
        hint.draw(Rectangle{static_cast<float>(paneX), static_cast<float>(paneY + 20), 260.0F,
                     static_cast<float>(kTextSize)},
            ui::format(ui::TextId::InventoryUnitsHintMore, static_cast<int>(pane.units.size() - rows)));
    } else {
        hint.draw(Rectangle{static_cast<float>(paneX), static_cast<float>(paneY + 20), 260.0F,
                     static_cast<float>(kTextSize)},
            ui::text(ui::TextId::InventoryUnitsHint));
    }

    for (std::size_t index = 0; index < rows; ++index) {
        drawUnitRow(pane, paneIndex, index);
    }
}

void InventoryPanelView::drawUnitRow(
    const events::InventoryPaneState& pane, std::size_t paneIndex, std::size_t index) const
{
    const events::UnitInventoryState& unit = pane.units[index];
    const Rectangle row
        = inventory_panel_layout::unitRowBounds(index, paneIndex, GetScreenWidth());
    ui::Text{ui::TextStyle{kUnitNameSize, RAYWHITE}}.draw(
        Rectangle{row.x, row.y, row.width, static_cast<float>(kUnitNameSize)}, ui::unitName(unit.type));
    ui::Text{ui::TextStyle{kUnitDetailSize, LIGHTGRAY}}.draw(
        Rectangle{row.x, row.y + 17.0F, row.width, static_cast<float>(kUnitDetailSize)},
        ui::format(ui::TextId::InventoryUnitHealth, unit.health, unit.maxHealth));

    for (std::size_t slotIndex = 0; slotIndex < unit.slots.size(); ++slotIndex) {
        const Rectangle bounds = inventory_panel_layout::unitSlotBounds(
            index, slotIndex, unit.slots.size(), paneIndex, GetScreenWidth());
        const events::InventorySlotState& slot = unit.slots[slotIndex];
        const ui::CardStyle style{
            kSlotColor,
            slot.occupied ? GRAY : Color{150, 70, 60, 255},
            1.0F,
        };
        ui::Card{style}.draw(bounds);
        if (!slot.occupied) {
            continue;
        }
        ui::Text{ui::TextStyle{kUnitSlotTextSize, RAYWHITE}}.draw(
            Rectangle{bounds.x + 2.0F, bounds.y + 2.0F, bounds.width - 4.0F, static_cast<float>(kUnitSlotTextSize)},
            ui::shortItemName(slot.item));
        ui::Text{ui::TextStyle{kUnitSlotTextSize, LIGHTGRAY}}.draw(
            Rectangle{bounds.x + 2.0F, bounds.y + 14.0F, bounds.width - 4.0F, static_cast<float>(kUnitSlotTextSize)},
            std::to_string(slot.currentQuantity));
    }
}

} // namespace view::gameplay
