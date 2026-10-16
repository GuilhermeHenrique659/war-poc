#include "view/PlayingView.hpp"

namespace view {

PlayingView::PlayingView(core::EventBus& bus)
    : world_(std::make_unique<WorldView>(bus))
    , playerHud_(std::make_unique<PlayerHudView>(bus))
    , buildBar_(std::make_unique<BuildBarView>(bus))
    , economyPanel_(std::make_unique<EconomyPanelView>(bus))
    , recruitPanel_(std::make_unique<RecruitPanelView>(bus))
{
}

void PlayingView::draw() const
{
    world_->draw();
    playerHud_->draw();
    economyPanel_->draw();
    buildBar_->draw();
    recruitPanel_->draw();
}

} // namespace view
