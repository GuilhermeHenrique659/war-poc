#include "view/menu/SettingsView.hpp"

#include "core/Events.hpp"
#include "view/menu/SettingsLayout.hpp"
#include "view/ui/Button.hpp"
#include "view/ui/Text.hpp"
#include "view/ui/TextCatalog.hpp"

#include <raylib.h>

#include <array>
#include <string>

namespace view::menu {

namespace {

constexpr int kTitleSize = 42;
constexpr int kRowTextSize = 24;
constexpr int kHintSize = 18;
constexpr ui::ButtonStyle kSettingsButtonStyle{
    Color{45, 48, 58, 255},
    Color{70, 75, 90, 255},
    Color{36, 36, 44, 255},
    GRAY,
    GOLD,
    RAYWHITE,
    2,
};

constexpr std::array kRowTexts{
    ui::TextId::SettingsResolution,
    ui::TextId::SettingsDisplayMode,
    ui::TextId::SettingsPlayerName,
    ui::TextId::SettingsApply,
    ui::TextId::SettingsBack,
};

} // namespace

SettingsView::SettingsView(core::EventBus& bus)
{
    bus.subscribe<events::SettingsStateChanged>([this](const events::SettingsStateChanged& state) {
        selectedIndex_ = state.selectedIndex;
        width_ = state.width;
        height_ = state.height;
        fullscreen_ = state.fullscreen;
        playerName_ = state.playerName;
    });
}

void SettingsView::draw() const
{
    const Rectangle titleBounds{0.0F, 100.0F, static_cast<float>(GetScreenWidth()), static_cast<float>(kTitleSize)};
    ui::Text{ui::TextStyle{kTitleSize, RAYWHITE, ui::TextAlign::Center}}.draw(
        titleBounds, ui::text(ui::TextId::SettingsTitle));

    for (std::size_t i = 0; i < settings_layout::kRowCount; ++i) {
        const Rectangle bounds = settings_layout::rowBounds(i, GetScreenWidth(), GetScreenHeight());
        const bool selected = i == selectedIndex_;

        std::string label = ui::text(kRowTexts[i]);
        if (i == 0) {
            label += "  <  " + ui::format(ui::TextId::SettingsResolutionValue, width_, height_) + "  >";
        } else if (i == 1) {
            label += fullscreen_ ? "  <  " + ui::text(ui::TextId::SettingsFullscreen) + "  >"
                                 : "  <  " + ui::text(ui::TextId::SettingsWindowed) + "  >";
        } else if (i == 2) {
            label += "  " + (playerName_.empty() ? ui::text(ui::TextId::SettingsDefaultPlayer) : playerName_);
        }
        const auto state = selected ? ui::ButtonState::Selected : ui::ButtonState::Normal;
        ui::Button{kSettingsButtonStyle}.draw(bounds, label, state, kRowTextSize);
    }

    const Rectangle hintBounds{0.0F, static_cast<float>(GetScreenHeight() - 58), static_cast<float>(GetScreenWidth()),
        static_cast<float>(kHintSize)};
    ui::Text{ui::TextStyle{kHintSize, LIGHTGRAY, ui::TextAlign::Center}}.draw(
        hintBounds, ui::text(ui::TextId::SettingsHint));
}

} // namespace view::menu
