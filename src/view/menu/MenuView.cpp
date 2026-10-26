#include "view/menu/MenuView.hpp"

#include "core/Events.hpp"
#include "view/menu/MenuLayout.hpp"
#include "view/ui/Button.hpp"
#include "view/ui/GameText.hpp"
#include "view/ui/Text.hpp"
#include "view/ui/TextCatalog.hpp"

#include <raylib.h>

namespace view::menu {

namespace {

constexpr int kTitleFontSize = 72;
constexpr int kItemFontSize = 28;
constexpr ui::ButtonStyle kMenuButtonStyle{
    Color{50, 50, 60, 255},
    Color{180, 60, 50, 255},
    Color{36, 36, 44, 255},
    GRAY,
    GOLD,
    RAYWHITE,
    2,
};

} // namespace

MenuView::MenuView(core::EventBus& bus)
{
    bus.subscribe<events::MenuStateChanged>([this](const events::MenuStateChanged& state) {
        actions_ = state.actions;
        selectedIndex_ = state.selectedIndex;
    });
}

void MenuView::draw() const
{
    drawTitle();
    for (std::size_t i = 0; i < actions_.size(); ++i) {
        drawItem(i);
    }
}

void MenuView::drawTitle() const
{
    const Rectangle first = menu_layout::itemBounds(0, actions_.size(), GetScreenWidth(), GetScreenHeight());
    const float y = first.y - menu_layout::kTitleOffsetY;
    const Rectangle titleBounds{0.0F, y, static_cast<float>(GetScreenWidth()), static_cast<float>(kTitleFontSize)};
    ui::Text{ui::TextStyle{kTitleFontSize, RAYWHITE, ui::TextAlign::Center}}.draw(
        titleBounds, ui::text(ui::TextId::MenuTitle));
}

void MenuView::drawItem(std::size_t index) const
{
    const Rectangle bounds = menu_layout::itemBounds(index, actions_.size(), GetScreenWidth(), GetScreenHeight());
    const auto state = index == selectedIndex_ ? ui::ButtonState::Selected : ui::ButtonState::Normal;
    ui::Button{kMenuButtonStyle}.draw(bounds, std::string{ui::menuLabel(actions_[index])}, state, kItemFontSize);
}

} // namespace view::menu
