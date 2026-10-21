#include "view/gameplay/PlayingView.hpp"

namespace view::gameplay {

PlayingView::PlayingView(core::EventBus& bus)
    : world_(std::make_unique<WorldView>(bus))
    , playerHud_(std::make_unique<PlayerHudView>(bus))
    , buildBar_(std::make_unique<BuildBarView>(bus))
    , inventoryPanel_(std::make_unique<InventoryPanelView>(bus))
    , recruitPanel_(std::make_unique<RecruitPanelView>(bus))
{
}

void PlayingView::draw() const
{
    world_->draw();
    playerHud_->draw();
    buildBar_->draw();
    recruitPanel_->draw();
    inventoryPanel_->draw();
}

} // namespace view::gameplay
