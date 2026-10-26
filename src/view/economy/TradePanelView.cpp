#include "view/economy/TradePanelView.hpp"

#include "model/build/BuildingType.hpp"
#include "view/economy/TradePanelLayout.hpp"
#include "view/ui/Button.hpp"
#include "view/ui/Card.hpp"
#include "view/ui/GameText.hpp"
#include "view/ui/Text.hpp"
#include "view/ui/TextCatalog.hpp"

#include <raylib.h>

namespace view::gameplay {
namespace {

constexpr Color kRowColor{48, 53, 64, 255};
constexpr Color kRowAltColor{42, 47, 57, 255};
constexpr int kTitleSize = 18;
constexpr int kRowSize = 14;

} // namespace

TradePanelView::TradePanelView(core::EventBus& bus)
{
    bus.subscribe<events::LocalPlayerChanged>(
        [this](const events::LocalPlayerChanged& event) { localPlayerId_ = event.player; });
    bus.subscribe<events::TradePanelChanged>([this](const events::TradePanelChanged& event) {
        if (event.player == localPlayerId_) {
            panel_ = event;
        }
    });
    bus.subscribe<events::WorldReset>([this](const events::WorldReset&) { panel_ = {}; });
}

void TradePanelView::draw() const
{
    if (!panel_.open) {
        return;
    }

    const int screenWidth = GetScreenWidth();
    const Rectangle bounds = trade_panel_layout::panelBounds(panel_.rows.size(), screenWidth);
    ui::Card{}.draw(bounds);

    const float titleX = bounds.x + trade_panel_layout::kPadding;
    const float titleY = bounds.y + 8.0F;
    ui::Text{ui::TextStyle{kTitleSize, RAYWHITE}}.draw(
        Rectangle{titleX, titleY, bounds.width - 2.0F * trade_panel_layout::kPadding,
            static_cast<float>(kTitleSize)},
        ui::buildingName(model::BuildingType::TradeCenter));

    const Rectangle close = trade_panel_layout::closeBounds(panel_.rows.size(), screenWidth);
    const ui::ButtonStyle closeStyle{
        Color{50, 50, 62, 255}, Color{170, 60, 50, 255}, Color{36, 36, 44, 255},
        GRAY, GRAY, RAYWHITE, 1};
    ui::Button{closeStyle}.draw(close, ui::text(ui::TextId::CommonClose),
        CheckCollisionPointRec(GetMousePosition(), close) ? ui::ButtonState::Selected : ui::ButtonState::Normal, 16);

    for (std::size_t index = 0; index < panel_.rows.size(); ++index) {
        const Rectangle row = trade_panel_layout::rowBounds(index, panel_.rows.size(), screenWidth);
        ui::Card{ui::CardStyle{index % 2U == 0U ? kRowColor : kRowAltColor, GRAY, 1.0F}}.draw(row);
        ui::Text{ui::TextStyle{kRowSize, RAYWHITE}}.draw(
            Rectangle{row.x + 7.0F, row.y + 5.0F, row.width - 14.0F, static_cast<float>(kRowSize)},
            ui::tradeRowText(panel_.rows[index]));
    }
}

} // namespace view::gameplay
