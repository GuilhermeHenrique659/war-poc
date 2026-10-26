#pragma once

#include "core/EventBus.hpp"
#include "model/menu/MenuAction.hpp"

#include <cstddef>
#include <vector>

namespace view::menu {

class MenuView {
public:
    explicit MenuView(core::EventBus& bus);

    void draw() const;

private:
    void drawTitle() const;
    void drawItem(std::size_t index) const;

    std::vector<model::MenuAction> actions_;
    std::size_t selectedIndex_ = 0;
};

} // namespace view::menu
