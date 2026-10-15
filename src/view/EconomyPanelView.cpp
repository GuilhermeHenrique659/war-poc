#include "view/EconomyPanelView.hpp"

#include "view/EconomyPanelLayout.hpp"

#include <raylib.h>

#include <array>

namespace view {

namespace {

constexpr Color kPanelColor{24, 24, 32, 235};
constexpr Color kPanelBorder{70, 70, 90, 255};
constexpr int kTitleSize = 18;
constexpr int kDetailSize = 16;
constexpr int kLineHeight = 23;
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
    DrawRectangleRec(panel, kPanelColor);
    DrawRectangleLinesEx(panel, 2.0f, kPanelBorder);

    const int x = static_cast<int>(panel.x + economy_panel_layout::kPadding);
    int y = static_cast<int>(panel.y + economy_panel_layout::kPadding);
    DrawText("Economia", x, y, kTitleSize, RAYWHITE);
    y += kLineHeight;

    for (std::size_t index = 0; index < kResourceNames.size(); ++index) {
        DrawText(TextFormat("%s %d / %d", kResourceNames[index], economy_.stock[index], economy_.capacity[index]),
            x, y, kDetailSize, LIGHTGRAY);
        y += kLineHeight;
    }
    DrawText(TextFormat("População %d / %d", economy_.populationUsed, economy_.populationCapacity),
        x, y, kDetailSize, RAYWHITE);
}

} // namespace view
