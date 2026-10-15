#include "model/GameModel.hpp"

namespace model {

GameModel::GameModel(core::EventBus& bus)
    : menu_(std::make_unique<MenuModel>(bus))
    , playing_(std::make_unique<PlayingModel>(bus))
{
}

void GameModel::initialize()
{
    menu_->initialize();
}

void GameModel::update(float deltaSeconds)
{
    playing_->update(deltaSeconds);
}

} // namespace model
