#include "view/menu/MenuView.hpp"

#include "core/Events.hpp"
#include "view/menu/MenuLayout.hpp"
#include "view/ui/Button.hpp"

#include <raylib.h>

namespace view::menu {

namespace {

constexpr const char* kTitle = "WAR";
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
        labels_ = state.labels;
        selectedIndex_ = state.selectedIndex;
    });
}

void MenuView::draw() const
{
    drawTitle();
    for (std::size_t i = 0; i < labels_.size(); ++i) {
        drawItem(i);
    }
}

void MenuView::drawTitle() const
{
    const Rectangle first = menu_layout::itemBounds(0, labels_.size(), GetScreenWidth(), GetScreenHeight());
    const int width = MeasureText(kTitle, kTitleFontSize);
    const int x = (GetScreenWidth() - width) / 2;
    const int y = static_cast<int>(first.y - menu_layout::kTitleOffsetY);
    DrawText(kTitle, x, y, kTitleFontSize, RAYWHITE);
}

void MenuView::drawItem(std::size_t index) const
{
    const Rectangle bounds = menu_layout::itemBounds(index, labels_.size(), GetScreenWidth(), GetScreenHeight());
    const auto state = index == selectedIndex_ ? ui::ButtonState::Selected : ui::ButtonState::Normal;
    ui::Button{kMenuButtonStyle}.draw(bounds, labels_[index], state, kItemFontSize);
}

} // namespace view::menu
