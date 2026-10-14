#include "view/PlayingView.hpp"

namespace view {

PlayingView::PlayingView(core::EventBus& bus)
    : world_(std::make_unique<WorldView>(bus))
    , buildBar_(std::make_unique<BuildBarView>(bus))
{
}

void PlayingView::draw() const
{
    world_->draw();
    buildBar_->draw();
}

} // namespace view
