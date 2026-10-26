#include "view/gameplay/PlayingView.hpp"

namespace view::gameplay {

PlayingView::PlayingView(core::EventBus& bus)
    : world_(std::make_unique<WorldView>(bus))
    , timeHud_(std::make_unique<TimeHudView>(bus))
    , playerHud_(std::make_unique<PlayerHudView>(bus))
    , buildBar_(std::make_unique<BuildBarView>(bus))
    , inventoryPanel_(std::make_unique<InventoryPanelView>(bus))
    , logisticsPanel_(std::make_unique<LogisticsPanelView>(bus))
    , tradePanel_(std::make_unique<TradePanelView>(bus))
    , recruitPanel_(std::make_unique<RecruitPanelView>(bus))
{
}

void PlayingView::draw()
{
    world_->draw();
    timeHud_->draw();
    playerHud_->draw();
    buildBar_->draw();
    recruitPanel_->draw();
    inventoryPanel_->draw();
    logisticsPanel_->draw();
    tradePanel_->draw();
}

} // namespace view::gameplay
