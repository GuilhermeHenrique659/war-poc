#include "view/economy/LogisticsPanelView.hpp"

#include "model/economy/LogisticsComponents.hpp"
#include "view/economy/LogisticsPanelLayout.hpp"
#include "view/ui/Button.hpp"
#include "view/ui/Card.hpp"
#include "view/ui/GameText.hpp"
#include "view/ui/Text.hpp"
#include "view/ui/TextCatalog.hpp"

#include <raylib.h>

#include <string>

namespace view::gameplay {
namespace {

constexpr Color kRowColor{48, 53, 64, 255};
constexpr Color kRowHover{67, 75, 88, 255};
constexpr Color kButtonColor{52, 100, 72, 255};
constexpr Color kDisabledColor{55, 55, 62, 255};
constexpr int kTitleSize = 18;
constexpr int kRouteSize = 14;
constexpr int kDetailSize = 13;
constexpr int kButtonSize = 12;

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

    const float titleX = bounds.x + logistics_panel_layout::kPadding;
    const float titleY = bounds.y + 8.0F;
    ui::Text{ui::TextStyle{kTitleSize, RAYWHITE}}.draw(
        Rectangle{titleX, titleY, bounds.width - 2.0F * logistics_panel_layout::kPadding, static_cast<float>(kTitleSize)},
        ui::buildingName(panel_.buildingType));
    const Rectangle close = logistics_panel_layout::closeBounds(panel_.routes.size(), panel_.warehouse, screenWidth);
    const ui::ButtonStyle closeStyle{
        Color{50, 50, 62, 255}, Color{170, 60, 50, 255}, Color{36, 36, 44, 255},
        GRAY, GRAY, RAYWHITE, 1};
    ui::Button{closeStyle}.draw(close, ui::text(ui::TextId::CommonClose),
        CheckCollisionPointRec(GetMousePosition(), close) ? ui::ButtonState::Selected : ui::ButtonState::Normal, 16);

    float nextY = bounds.y + logistics_panel_layout::kPadding + logistics_panel_layout::kHeaderHeight;
    if (panel_.warehouse) {
        const Rectangle recruit = logistics_panel_layout::recruitBounds(panel_.routes.size(), screenWidth);
        const ui::ButtonStyle recruitStyle{
            kButtonColor, Color{72, 130, 92, 255}, Color{38, 76, 54, 255},
            GRAY, GRAY, RAYWHITE, 1};
        const std::string label = panel_.free
            ? ui::format(ui::TextId::LogisticsRecruitFree, panel_.idleDrones)
            : ui::format(ui::TextId::LogisticsRecruitCost, model::components::kTransportDroneIronCost,
                model::components::kTransportDroneElectronicsCost, panel_.idleDrones);
        ui::Button{recruitStyle}.draw(recruit, label,
            CheckCollisionPointRec(GetMousePosition(), recruit) ? ui::ButtonState::Selected : ui::ButtonState::Normal,
            kButtonSize);
        nextY += logistics_panel_layout::kRecruitHeight;
    }

    if (panel_.routes.empty()) {
        ui::Text{ui::TextStyle{kRouteSize, LIGHTGRAY}}.draw(
            Rectangle{titleX, nextY + 4.0F, bounds.width - 2.0F * logistics_panel_layout::kPadding,
                static_cast<float>(kRouteSize)},
            ui::text(panel_.warehouse ? ui::TextId::LogisticsEmptyWarehouse : ui::TextId::LogisticsEmptySource));
        return;
    }

    for (std::size_t index = 0; index < panel_.routes.size(); ++index) {
        const events::LogisticsRouteEntry& route = panel_.routes[index];
        const Rectangle row = logistics_panel_layout::routeBounds(index, panel_.routes.size(), screenWidth);
        const bool hovered = CheckCollisionPointRec(GetMousePosition(), row);
        ui::Card{ui::CardStyle{hovered ? kRowHover : kRowColor, hovered ? GOLD : GRAY, 1.0F}}.draw(row);

        ui::Text{ui::TextStyle{kRouteSize, RAYWHITE}}.draw(
            Rectangle{row.x + 7.0F, row.y + 5.0F, row.width - 14.0F, static_cast<float>(kRouteSize)},
            ui::format(ui::TextId::LogisticsRoute, route.id, ui::buildingName(route.destinationType)));
        ui::Text{ui::TextStyle{kDetailSize, LIGHTGRAY}}.draw(
            Rectangle{row.x + 7.0F, row.y + 27.0F, row.width - 14.0F, static_cast<float>(kDetailSize)},
            ui::format(ui::TextId::LogisticsRouteItems, ui::itemName(route.item), route.assignedDrones));

        const Rectangle assign = logistics_panel_layout::assignBounds(index, panel_.routes.size(), screenWidth);
        const bool canAssign = panel_.idleDrones > 0;
        const ui::ButtonStyle assignStyle{
            canAssign ? kButtonColor : kDisabledColor, Color{72, 130, 92, 255},
            Color{38, 76, 54, 255}, GRAY, GRAY, RAYWHITE, 1};
        const bool assignHovered = CheckCollisionPointRec(GetMousePosition(), assign);
        ui::Button{assignStyle}.draw(assign, ui::text(ui::TextId::LogisticsAssign),
            assignHovered && canAssign ? ui::ButtonState::Selected : ui::ButtonState::Normal, kDetailSize);

        const Rectangle unassign = logistics_panel_layout::unassignBounds(index, panel_.routes.size(), screenWidth);
        const bool canUnassign = route.assignedDrones > 0;
        const ui::ButtonStyle unassignStyle{
            canUnassign ? Color{125, 75, 55, 255} : kDisabledColor, Color{155, 95, 70, 255},
            Color{90, 50, 40, 255}, GRAY, GRAY, RAYWHITE, 1};
        const bool unassignHovered = CheckCollisionPointRec(GetMousePosition(), unassign);
        ui::Button{unassignStyle}.draw(unassign, ui::text(ui::TextId::LogisticsUnassign),
            unassignHovered && canUnassign ? ui::ButtonState::Selected : ui::ButtonState::Normal, kDetailSize);
    }
}

} // namespace view::gameplay
