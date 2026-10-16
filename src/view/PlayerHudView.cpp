#include "view/PlayerHudView.hpp"

#include "view/PlayerHudLayout.hpp"

#include <algorithm>

namespace view {

namespace {

constexpr Color kPanelColor{24, 24, 32, 235};
constexpr Color kPanelBorder{70, 70, 90, 255};
constexpr Color kHealthBack{45, 25, 25, 255};
constexpr Color kHealthFill{90, 220, 110, 255};
constexpr int kTitleSize = 18;
constexpr int kHealthSize = 16;

} // namespace

PlayerHudView::PlayerHudView(core::EventBus& bus)
{
    bus.subscribe<events::LocalPlayerChanged>([this](const events::LocalPlayerChanged& event) {
        localPlayerId_ = event.player;
        player_ = {};
        hasPlayer_ = false;
    });
    bus.subscribe<events::PlayerSpawned>([this](const events::PlayerSpawned& event) {
        if (event.player != localPlayerId_) {
            return;
        }
        player_ = event;
        hasPlayer_ = true;
    });
    bus.subscribe<events::HealthChanged>([this](const events::HealthChanged& event) {
        if (!hasPlayer_ || event.entity != player_.entity) {
            return;
        }
        player_.health = event.health;
        player_.maxHealth = event.maxHealth;
    });
    bus.subscribe<events::WorldReset>([this](const events::WorldReset&) {
        player_ = {};
        hasPlayer_ = false;
    });
}

void PlayerHudView::draw() const
{
    if (!hasPlayer_) {
        return;
    }

    const Rectangle panel = player_hud_layout::panelBounds(GetScreenWidth());
    DrawRectangleRec(panel, kPanelColor);
    DrawRectangleLinesEx(panel, 2.0F, kPanelBorder);

    const int x = static_cast<int>(panel.x + 10.0F);
    const int y = static_cast<int>(panel.y + 8.0F);
    DrawText("Jogador", x, y, kTitleSize, RAYWHITE);
    DrawText(TextFormat("Vida %d / %d", player_.health, player_.maxHealth), x, y + 25, kHealthSize, LIGHTGRAY);

    const float barX = panel.x + 10.0F;
    const float barY = panel.y + 52.0F;
    const float barWidth = panel.width - 20.0F;
    const Rectangle bar{barX, barY, barWidth, 10.0F};
    DrawRectangleRec(bar, kHealthBack);
    const float ratio = player_.maxHealth > 0
        ? std::clamp(static_cast<float>(player_.health) / static_cast<float>(player_.maxHealth), 0.0F, 1.0F)
        : 0.0F;
    DrawRectangleRec(Rectangle{bar.x, bar.y, bar.width * ratio, bar.height}, kHealthFill);
    DrawRectangleLinesEx(bar, 1.0F, BLACK);
}

} // namespace view
