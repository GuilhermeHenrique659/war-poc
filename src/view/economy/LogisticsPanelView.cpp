#include "view/economy/LogisticsPanelView.hpp"

#include "model/economy/Item.hpp"
#include "model/economy/LogisticsComponents.hpp"
#include "view/economy/LogisticsPanelLayout.hpp"
#include "view/ui/Button.hpp"
#include "view/ui/Card.hpp"

#include <raylib.h>

#include <string>

namespace view::gameplay {
namespace {

constexpr Color kRowColor{48, 53, 64, 255};
constexpr Color kRowHover{67, 75, 88, 255};
constexpr Color kButtonColor{52, 100, 72, 255};
constexpr Color kDisabledColor{55, 55, 62, 255};

[[nodiscard]] const char* itemLabel(model::ItemId item)
{
    switch (item) {
    case model::ItemId::Iron: return "Ferro";
    case model::ItemId::Coal: return "Carvao";
    case model::ItemId::Steel: return "Aco";
    case model::ItemId::Copper: return "Cobre";
    case model::ItemId::Electronics: return "Eletronicos";
    case model::ItemId::Tools: return "Ferramentas";
    case model::ItemId::Count: return "?";
    }
    return "?";
}

} // namespace

LogisticsPanelView::LogisticsPanelView(core::EventBus& bus)
{
    bus.subscribe<events::LocalPlayerChanged>(
        [this](const events::LocalPlayerChanged& event) { localPlayerId_ = event.player; });
    bus.subscribe<events::LogisticsPanelChanged>([this](const events::LogisticsPanelChanged& event) {
        if (event.player == localPlayerId_) {
            panel_ = event;
        }
    });
    bus.subscribe<events::DebugToggleRequested>([this](const events::DebugToggleRequested&) {
        debugMode_ = !debugMode_;
    });
    bus.subscribe<events::WorldReset>([this](const events::WorldReset&) { panel_ = {}; });
}

void LogisticsPanelView::draw() const
{
    if (!panel_.open) {
        return;
    }

    const int screenWidth = GetScreenWidth();
    const Rectangle bounds = logistics_panel_layout::panelBounds(panel_.routes.size(), panel_.warehouse, screenWidth);
    ui::Card{}.draw(bounds);

    const int titleX = static_cast<int>(bounds.x + logistics_panel_layout::kPadding);
    const int titleY = static_cast<int>(bounds.y + 8.0F);
    DrawText(panel_.buildingName.c_str(), titleX, titleY, 18, RAYWHITE);
    const Rectangle close = logistics_panel_layout::closeBounds(panel_.routes.size(), panel_.warehouse, screenWidth);
    const ui::ButtonStyle closeStyle{
        Color{50, 50, 62, 255}, Color{170, 60, 50, 255}, Color{36, 36, 44, 255},
        GRAY, GRAY, RAYWHITE, 1};
    ui::Button{closeStyle}.draw(close, "X",
        CheckCollisionPointRec(GetMousePosition(), close) ? ui::ButtonState::Selected : ui::ButtonState::Normal, 16);

    float nextY = bounds.y + logistics_panel_layout::kPadding + logistics_panel_layout::kHeaderHeight;
    if (panel_.warehouse) {
        const Rectangle recruit = logistics_panel_layout::recruitBounds(panel_.routes.size(), screenWidth);
        const ui::ButtonStyle recruitStyle{
            kButtonColor, Color{72, 130, 92, 255}, Color{38, 76, 54, 255},
            GRAY, GRAY, RAYWHITE, 1};
        ui::Button{recruitStyle}.draw(recruit,
            debugMode_
                ? TextFormat("Drone: Gratis (ociosos: %d)", panel_.idleDrones)
                : TextFormat("Drone: %d ferro + %d eletr. (ociosos: %d)",
                    model::components::kTransportDroneIronCost, model::components::kTransportDroneElectronicsCost,
                    panel_.idleDrones),
            CheckCollisionPointRec(GetMousePosition(), recruit) ? ui::ButtonState::Selected : ui::ButtonState::Normal, 12);
        nextY += logistics_panel_layout::kRecruitHeight;
    }

    if (panel_.routes.empty()) {
        DrawText(panel_.warehouse ? "Nenhuma rota para este armazem." : "Nenhuma rota de saida.",
            titleX, static_cast<int>(nextY + 4.0F), 14, LIGHTGRAY);
        return;
    }

    for (std::size_t index = 0; index < panel_.routes.size(); ++index) {
        const events::LogisticsRouteEntry& route = panel_.routes[index];
        const Rectangle row = logistics_panel_layout::routeBounds(index, panel_.routes.size(), screenWidth);
        const bool hovered = CheckCollisionPointRec(GetMousePosition(), row);
        ui::Card{ui::CardStyle{hovered ? kRowHover : kRowColor, hovered ? GOLD : GRAY, 1.0F}}.draw(row);

        const std::string label = "Rota #" + std::to_string(route.id) + " -> " + route.destinationName;
        DrawText(label.c_str(), static_cast<int>(row.x + 7.0F), static_cast<int>(row.y + 5.0F), 14, RAYWHITE);
        DrawText(TextFormat("%s  |  Drones: %d", itemLabel(route.item), route.assignedDrones),
            static_cast<int>(row.x + 7.0F), static_cast<int>(row.y + 27.0F), 13, LIGHTGRAY);

        const Rectangle assign = logistics_panel_layout::assignBounds(index, panel_.routes.size(), screenWidth);
        const bool canAssign = panel_.idleDrones > 0;
        const ui::ButtonStyle assignStyle{
            canAssign ? kButtonColor : kDisabledColor, Color{72, 130, 92, 255},
            Color{38, 76, 54, 255}, GRAY, GRAY, RAYWHITE, 1};
        const bool assignHovered = CheckCollisionPointRec(GetMousePosition(), assign);
        ui::Button{assignStyle}.draw(assign, "Atribuir",
            assignHovered && canAssign ? ui::ButtonState::Selected : ui::ButtonState::Normal, 13);

        const Rectangle unassign = logistics_panel_layout::unassignBounds(index, panel_.routes.size(), screenWidth);
        const bool canUnassign = route.assignedDrones > 0;
        const ui::ButtonStyle unassignStyle{
            canUnassign ? Color{125, 75, 55, 255} : kDisabledColor, Color{155, 95, 70, 255},
            Color{90, 50, 40, 255}, GRAY, GRAY, RAYWHITE, 1};
        const bool unassignHovered = CheckCollisionPointRec(GetMousePosition(), unassign);
        ui::Button{unassignStyle}.draw(unassign, "Remover",
            unassignHovered && canUnassign ? ui::ButtonState::Selected : ui::ButtonState::Normal, 13);
    }
}

} // namespace view::gameplay
