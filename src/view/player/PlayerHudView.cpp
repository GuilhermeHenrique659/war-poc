#include "view/player/PlayerHudView.hpp"

#include "view/player/PlayerHudLayout.hpp"
#include "view/ui/Bar.hpp"
#include "view/ui/Card.hpp"
#include "view/ui/Text.hpp"
#include "view/ui/TextCatalog.hpp"

#include <raylib.h>

#include <algorithm>

namespace view::gameplay {

namespace {

constexpr Color kHealthBack{45, 25, 25, 255};
constexpr Color kHealthFill{90, 220, 110, 255};
constexpr int kTitleSize = 18;
constexpr int kHealthSize = 16;
constexpr ui::BarStyle kHealthBarStyle{kHealthBack, kHealthFill, BLACK, Color{0, 0, 0, 0}, 1.0F, 0.0F};

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
    ui::Card{}.draw(panel);

    const Rectangle titleBounds{panel.x + 10.0F, panel.y + 8.0F, panel.width - 20.0F,
        static_cast<float>(kTitleSize)};
    ui::Text{ui::TextStyle{kTitleSize, RAYWHITE}}.draw(titleBounds, ui::text(ui::TextId::HudPlayer));

    const Rectangle healthBounds{panel.x + 10.0F, panel.y + 33.0F, panel.width - 20.0F,
        static_cast<float>(kHealthSize)};
    ui::Text{ui::TextStyle{kHealthSize, LIGHTGRAY}}.draw(
        healthBounds, ui::format(ui::TextId::HudHealth, player_.health, player_.maxHealth));

    const Rectangle bar{panel.x + 10.0F, panel.y + 52.0F, panel.width - 20.0F, 10.0F};
    const float ratio = player_.maxHealth > 0
        ? std::clamp(static_cast<float>(player_.health) / static_cast<float>(player_.maxHealth), 0.0F, 1.0F)
        : 0.0F;
    ui::Bar{kHealthBarStyle}.draw(bar, ratio);
}

} // namespace view::gameplay
