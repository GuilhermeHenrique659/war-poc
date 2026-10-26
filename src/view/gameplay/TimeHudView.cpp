#include "view/gameplay/TimeHudView.hpp"

#include "view/gameplay/TimeHudLayout.hpp"
#include "view/ui/Card.hpp"
#include "view/ui/GameText.hpp"
#include "view/ui/Text.hpp"

#include <raylib.h>

namespace view::gameplay {

namespace {

constexpr int kDateFontSize = 14;
constexpr int kClockFontSize = 18;
constexpr float kPadding = 10.0F;
constexpr float kDateTop = 8.0F;
constexpr float kClockTop = 28.0F;

} // namespace

TimeHudView::TimeHudView(core::EventBus& bus)
{
    bus.subscribe<events::TimeChanged>([this](const events::TimeChanged& event) {
        time_ = event;
        hasTime_ = true;
    });
    bus.subscribe<events::WorldReset>([this](const events::WorldReset&) {
        time_ = {};
        hasTime_ = false;
    });
}

void TimeHudView::draw() const
{
    if (!hasTime_) {
        return;
    }

    const Rectangle panel = time_hud_layout::panelBounds(GetScreenWidth());
    ui::Card{}.draw(panel);

    const float contentWidth = panel.width - 2.0F * kPadding;
    const Rectangle dateBounds{
        panel.x + kPadding, panel.y + kDateTop, contentWidth, static_cast<float>(kDateFontSize)};
    ui::Text{ui::TextStyle{kDateFontSize, LIGHTGRAY}}.draw(dateBounds, ui::dateText(time_));

    const Rectangle clockBounds{
        panel.x + kPadding, panel.y + kClockTop, contentWidth, static_cast<float>(kClockFontSize)};
    ui::Text{ui::TextStyle{kClockFontSize, RAYWHITE}}.draw(clockBounds, ui::dayClockText(time_));
}

} // namespace view::gameplay
