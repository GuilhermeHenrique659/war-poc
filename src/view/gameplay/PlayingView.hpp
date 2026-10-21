#pragma once

#include "core/EventBus.hpp"
#include "view/build/BuildBarView.hpp"
#include "view/economy/InventoryPanelView.hpp"
#include "view/player/PlayerHudView.hpp"
#include "view/units/RecruitPanelView.hpp"
#include "view/world/WorldView.hpp"

#include <memory>

namespace view::gameplay {

class PlayingView {
public:
    explicit PlayingView(core::EventBus& bus);

    void draw() const;

private:
    std::unique_ptr<WorldView> world_;
    std::unique_ptr<PlayerHudView> playerHud_;
    std::unique_ptr<BuildBarView> buildBar_;
    std::unique_ptr<InventoryPanelView> inventoryPanel_;
    std::unique_ptr<RecruitPanelView> recruitPanel_;
};

} // namespace view::gameplay
