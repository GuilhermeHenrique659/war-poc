#include "view/MenuView.hpp"

#include "core/Events.hpp"
#include "view/MenuLayout.hpp"

#include <raylib.h>

namespace view {

namespace {

constexpr const char* kTitle = "WAR";
constexpr int kTitleFontSize = 72;
constexpr int kItemFontSize = 28;

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
    const bool selected = index == selectedIndex_;

    DrawRectangleRec(bounds, selected ? Color{180, 60, 50, 255} : Color{50, 50, 60, 255});
    DrawRectangleLinesEx(bounds, 2.0f, selected ? GOLD : GRAY);

    const char* label = labels_[index].c_str();
    const int textWidth = MeasureText(label, kItemFontSize);
    const int x = static_cast<int>(bounds.x + (bounds.width - static_cast<float>(textWidth)) / 2.0f);
    const int y = static_cast<int>(bounds.y + (bounds.height - static_cast<float>(kItemFontSize)) / 2.0f);
    DrawText(label, x, y, kItemFontSize, RAYWHITE);
}

} // namespace view
