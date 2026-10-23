#include "view/economy/InventoryPanelView.hpp"

#include "model/economy/Item.hpp"
#include "view/economy/InventoryPanelLayout.hpp"
#include "view/ui/Button.hpp"
#include "view/ui/Card.hpp"

#include <raylib.h>

#include <algorithm>
#include <cmath>
#include <string>

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

[[nodiscard]] std::string formatQuantity(int quantity)
{
    return std::to_string(quantity);
}

// Unit slots are narrow, so weapon/ammo use compact labels; other items keep their full name.
[[nodiscard]] std::string unitSlotLabel(model::ItemId item)
{
    switch (item) {
    case model::ItemId::Weapon:
        return "Arma";
    case model::ItemId::Ammo:
        return "Mun";
    default:
        return std::string(itemDefinition(item).name);
    }
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
    const int x = static_cast<int>(panelBounds.x + inventory_panel_layout::kPadding);
    const int y = static_cast<int>(panelBounds.y + inventory_panel_layout::kPadding);
    DrawText("Inventario", x, y, kTitleSize, RAYWHITE);

    const Rectangle closeBounds = inventory_panel_layout::closeBounds(GetScreenWidth());
    const bool closeHovered = CheckCollisionPointRec(GetMousePosition(), closeBounds);
    const ui::ButtonStyle closeStyle{
        kSlotColor, Color{170, 60, 50, 255}, Color{36, 36, 44, 255}, GRAY, GRAY, RAYWHITE, 1};
    ui::Button{closeStyle}.draw(
        closeBounds, "X", closeHovered ? ui::ButtonState::Selected : ui::ButtonState::Normal, kTextSize);

    for (std::size_t index = 0; index < panel_.panes.size(); ++index) {
        drawPane(panel_.panes[index], index);
    }

    if (drag_.active) {
        const Vector2 mouse = GetMousePosition();
        const Rectangle cursor{mouse.x + 12.0F, mouse.y + 12.0F, 105.0F, 36.0F};
        DrawRectangleRec(cursor, Color{20, 20, 28, 230});
        DrawRectangleLinesEx(cursor, 1.0F, GOLD);
        const std::string label = std::string(itemDefinition(drag_.item).name) + " "
            + formatQuantity(drag_.quantity);
        DrawText(label.c_str(), static_cast<int>(cursor.x + 5.0F), static_cast<int>(cursor.y + 10.0F),
            kTextSize, RAYWHITE);
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
    DrawText(TextFormat("%s (%d/%d)", pane.title.c_str(), usedSlots, static_cast<int>(pane.slots.size())),
        paneX, paneY, kTextSize, RAYWHITE);

    if (pane.showSummary) {
        for (std::size_t index = 0; index < model::kAllItems.size(); ++index) {
            const model::ItemId item = model::kAllItems[index];
            const std::string value = std::string(model::itemDefinition(item).name) + " "
                + formatQuantity(itemAmount(pane.slots, item));
            const std::size_t column = index % 2;
            const std::size_t row = index / 2;
            const int summaryX = paneX + static_cast<int>(column) * 144;
            DrawText(value.c_str(), summaryX, paneY + 20 + static_cast<int>(row) * 18, kTextSize, LIGHTGRAY);
        }
    }
    if (pane.showTransferHint) {
        DrawText("Arraste itens para transferir", paneX, paneY + (pane.showSummary ? 96 : 62),
            kTextSize, LIGHTGRAY);
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
    const std::string name(itemDefinition(slot.item).name);
    DrawText(name.c_str(), static_cast<int>(bounds.x + 2.0F), static_cast<int>(bounds.y + 4.0F),
        kSlotTextSize, RAYWHITE);
    const std::string quantity = formatQuantity(slot.currentQuantity);
    DrawText(quantity.c_str(), static_cast<int>(bounds.x + 2.0F), static_cast<int>(bounds.y + 17.0F),
        kSlotTextSize, LIGHTGRAY);
}

void InventoryPanelView::drawUnitsPane(
    const events::InventoryPaneState& pane, std::size_t paneIndex) const
{
    const int screenWidth = GetScreenWidth();
    const int paneX = static_cast<int>(inventory_panel_layout::paneBounds(paneIndex, screenWidth).x);
    const int paneY = static_cast<int>(inventory_panel_layout::panelBounds(screenWidth).y + 44.0F);

    DrawText(TextFormat("%s (%d)", pane.title.c_str(), static_cast<int>(pane.units.size())), paneX,
        paneY, kTextSize, RAYWHITE);
    const std::size_t rows
        = std::min(pane.units.size(), inventory_panel_layout::kMaxUnitRows);
    if (pane.units.size() > rows) {
        DrawText(TextFormat("Arma e municao por tropa (+%d)", static_cast<int>(pane.units.size() - rows)),
            paneX, paneY + 20, kTextSize, LIGHTGRAY);
    } else {
        DrawText("Arma e municao por tropa", paneX, paneY + 20, kTextSize, LIGHTGRAY);
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
    DrawText(unit.label.c_str(), static_cast<int>(row.x), static_cast<int>(row.y), kUnitNameSize, RAYWHITE);
    DrawText(TextFormat("Vida %d/%d", unit.health, unit.maxHealth), static_cast<int>(row.x),
        static_cast<int>(row.y + 17.0F), kUnitDetailSize, LIGHTGRAY);

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
        const std::string name = unitSlotLabel(slot.item);
        DrawText(name.c_str(), static_cast<int>(bounds.x + 2.0F), static_cast<int>(bounds.y + 2.0F),
            kUnitSlotTextSize, RAYWHITE);
        const std::string quantity = formatQuantity(slot.currentQuantity);
        DrawText(quantity.c_str(), static_cast<int>(bounds.x + 2.0F), static_cast<int>(bounds.y + 14.0F),
            kUnitSlotTextSize, LIGHTGRAY);
    }
}

} // namespace view::gameplay
