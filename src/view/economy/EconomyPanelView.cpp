#include "view/economy/EconomyPanelView.hpp"

#include "view/economy/EconomyPanelLayout.hpp"
#include "view/ui/Card.hpp"
#include "view/ui/Table.hpp"

#include <raylib.h>

#include <array>
#include <string>
#include <vector>

namespace view::gameplay {

namespace {

constexpr int kTitleSize = 18;
constexpr std::array<const char*, 4> kResourceNames{"Ouro", "Madeira", "Aço", "Petróleo"};

} // namespace

EconomyPanelView::EconomyPanelView(core::EventBus& bus)
{
    bus.subscribe<events::LocalPlayerChanged>([this](const events::LocalPlayerChanged& event) {
        localPlayerId_ = event.player;
        hasEconomy_ = false;
    });
    bus.subscribe<events::EconomyChanged>([this](const events::EconomyChanged& event) {
        if (event.player == localPlayerId_) {
            economy_ = event;
            hasEconomy_ = true;
        }
    });
    bus.subscribe<events::WorldReset>([this](const events::WorldReset&) {
        economy_ = {};
        hasEconomy_ = false;
    });
}

void EconomyPanelView::draw() const
{
    if (!hasEconomy_) {
        return;
    }

    const Rectangle panel = economy_panel_layout::panelBounds(GetScreenWidth());
    ui::Card{}.draw(panel);

    const int x = static_cast<int>(panel.x + economy_panel_layout::kPadding);
    const int y = static_cast<int>(panel.y + economy_panel_layout::kPadding);
    DrawText("Economia", x, y, kTitleSize, RAYWHITE);

    std::vector<ui::TableRow> rows;
    rows.reserve(kResourceNames.size() + 1);
    for (std::size_t index = 0; index < kResourceNames.size(); ++index) {
        rows.push_back(ui::TableRow{
            kResourceNames[index],
            std::to_string(economy_.stock[index]) + " / " + std::to_string(economy_.capacity[index]),
        });
    }
    rows.push_back(ui::TableRow{
        "População",
        std::to_string(economy_.populationUsed) + " / " + std::to_string(economy_.populationCapacity),
        RAYWHITE,
    });

    const float tableTop = panel.y + economy_panel_layout::kPadding + 28.0F;
    const float tableWidth = panel.width - 2.0F * economy_panel_layout::kPadding;
    ui::Table{ui::TableStyle{16, 23.0F, LIGHTGRAY, Color{70, 70, 90, 180}}}
        .draw(Rectangle{static_cast<float>(x), tableTop, tableWidth, panel.height}, rows);
}

} // namespace view::gameplay
