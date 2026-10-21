#include "view/economy/InventoryPanelView.hpp"

#include "model/economy/Item.hpp"
#include "view/economy/InventoryPanelLayout.hpp"
#include "view/ui/Button.hpp"
#include "view/ui/Card.hpp"

#include <raylib.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

namespace view::gameplay {
namespace {

constexpr Color kSlotColor{50, 50, 62, 255};
constexpr Color kSlotHover{72, 105, 78, 255};
constexpr int kTitleSize = 18;
constexpr int kTextSize = 14;
constexpr int kSlotTextSize = 10;

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

    if (panel_.playerVisible) {
        const Rectangle playerPane = inventory_panel_layout::paneBounds(true, GetScreenWidth());
        const int paneX = static_cast<int>(playerPane.x);
        const int paneY = static_cast<int>(panelBounds.y + 44.0F);
        const auto usedSlots = static_cast<int>(std::count_if(
            panel_.playerSlots.begin(), panel_.playerSlots.end(), [](const auto& slot) { return slot.occupied; }));
        DrawText(TextFormat("Jogador (%d/32)", usedSlots), paneX, paneY, kTextSize, RAYWHITE);
        const std::array<model::ItemId, 3> items{
            model::ItemId::Iron, model::ItemId::Coal, model::ItemId::Steel};
        const std::array<const char*, 3> names{"Ferro", "Carvao", "Aco"};
        int summaryX = paneX;
        for (std::size_t index = 0; index < items.size(); ++index) {
            const std::string value = std::string(names[index]) + " "
                + formatQuantity(itemAmount(panel_.playerSlots, items[index]));
            DrawText(value.c_str(), summaryX, paneY + 20, kTextSize, LIGHTGRAY);
            summaryX += MeasureText(value.c_str(), kTextSize) + 8;
        }
        DrawText("Arraste itens para transferir", paneX, paneY + 42, kTextSize, LIGHTGRAY);
        drawInventory(panel_.playerSlots, true);
    }

    if (panel_.hasBuilding) {
        const Rectangle buildingPane = inventory_panel_layout::paneBounds(false, GetScreenWidth());
        const int paneX = static_cast<int>(buildingPane.x);
        const int paneY = static_cast<int>(panelBounds.y + 44.0F);
        const auto usedSlots = static_cast<int>(std::count_if(
            panel_.buildingSlots.begin(), panel_.buildingSlots.end(), [](const auto& slot) { return slot.occupied; }));
        DrawText(TextFormat("%s (%d/64)", panel_.buildingName.c_str(), usedSlots), paneX, paneY, kTextSize, RAYWHITE);
        DrawText("Arraste itens para transferir", paneX, paneY + 62, kTextSize, LIGHTGRAY);
        drawInventory(panel_.buildingSlots, false);
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

void InventoryPanelView::drawInventory(
    const std::vector<events::InventorySlotState>& slots, bool playerInventory) const
{
    for (std::size_t index = 0; index < slots.size(); ++index) {
        drawSlot(index, playerInventory);
    }
}

void InventoryPanelView::drawSlot(std::size_t index, bool playerInventory) const
{
    const auto& slots = playerInventory ? panel_.playerSlots : panel_.buildingSlots;
    const Rectangle bounds = inventory_panel_layout::slotBounds(index, playerInventory, GetScreenWidth());
    const bool hovered = CheckCollisionPointRec(GetMousePosition(), bounds);
    const bool occupied = slots[index].occupied;
    const ui::CardStyle style{
        occupied && hovered ? kSlotHover : kSlotColor,
        hovered ? GOLD : GRAY,
        1.0F,
    };
    ui::Card{style}.draw(bounds);
    if (!occupied) {
        return;
    }

    const auto& slot = slots[index];
    const std::string name(itemDefinition(slot.item).name);
    DrawText(name.c_str(), static_cast<int>(bounds.x + 2.0F), static_cast<int>(bounds.y + 4.0F),
        kSlotTextSize, RAYWHITE);
    const std::string quantity = formatQuantity(slot.currentQuantity);
    DrawText(quantity.c_str(), static_cast<int>(bounds.x + 2.0F), static_cast<int>(bounds.y + 17.0F),
        kSlotTextSize, LIGHTGRAY);
}

} // namespace view::gameplay
