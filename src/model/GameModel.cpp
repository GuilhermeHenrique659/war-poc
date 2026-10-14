#include "model/GameModel.hpp"

namespace model {

GameModel::GameModel(core::EventBus& bus)
    : bus_(bus)
    , menuItems_{{"Iniciar", MenuAction::StartGame}, {"Sair", MenuAction::Quit}}
    , world_(std::make_unique<ecs::Registry>())
    , buildings_(std::make_unique<BuildingSystem>(bus_, *world_))
{
    bus_.subscribe<events::MenuNavigateRequested>([this](const auto& e) { onMenuNavigate(e); });
    bus_.subscribe<events::MenuItemHovered>([this](const auto& e) { onMenuItemHovered(e); });
    bus_.subscribe<events::MenuConfirmRequested>([this](const auto& e) { onMenuConfirm(e); });
    bus_.subscribe<events::ReturnToMenuRequested>([this](const auto& e) { onReturnToMenu(e); });
    subscribeBuildCommands();
}

void GameModel::subscribeBuildCommands()
{
    bus_.subscribe<events::BuildSelectRequested>([this](const events::BuildSelectRequested& e) {
        if (inMatch()) {
            buildings_->select(e.player, e.type);
        }
    });
    bus_.subscribe<events::BuildCancelRequested>([this](const events::BuildCancelRequested& e) {
        if (inMatch()) {
            buildings_->cancel(e.player);
        }
    });
    bus_.subscribe<events::BuildCursorMoved>([this](const events::BuildCursorMoved& e) {
        if (inMatch()) {
            buildings_->moveCursor(e.player, e.tile);
        }
    });
    bus_.subscribe<events::BuildPlaceRequested>([this](const events::BuildPlaceRequested& e) {
        if (inMatch()) {
            buildings_->place(e.player, e.tile);
        }
    });
}

void GameModel::initialize()
{
    changeScene(Scene::MainMenu);
    publishMenuState();
}

void GameModel::update(float /*deltaSeconds*/)
{
    if (scene_ != Scene::Playing) {
        return;
    }
    // Gameplay systems will run over world_ here.
}

void GameModel::onMenuNavigate(const events::MenuNavigateRequested& event)
{
    if (scene_ != Scene::MainMenu || menuItems_.empty()) {
        return;
    }

    const std::size_t count = menuItems_.size();
    selectedIndex_ = event.direction == events::MenuDirection::Down
        ? (selectedIndex_ + 1) % count
        : (selectedIndex_ + count - 1) % count;
    publishMenuState();
}

void GameModel::onMenuItemHovered(const events::MenuItemHovered& event)
{
    if (scene_ != Scene::MainMenu || event.index >= menuItems_.size()) {
        return;
    }
    if (event.index == selectedIndex_) {
        return;
    }

    selectedIndex_ = event.index;
    publishMenuState();
}

void GameModel::onMenuConfirm(const events::MenuConfirmRequested& /*event*/)
{
    if (scene_ != Scene::MainMenu || selectedIndex_ >= menuItems_.size()) {
        return;
    }

    switch (menuItems_[selectedIndex_].action) {
    case MenuAction::StartGame:
        startMatch();
        return;
    case MenuAction::Quit:
        bus_.publish(events::QuitRequested{});
        return;
    }
}

void GameModel::onReturnToMenu(const events::ReturnToMenuRequested& /*event*/)
{
    if (scene_ != Scene::Playing) {
        return;
    }

    resetWorld();
    changeScene(Scene::MainMenu);
    publishMenuState();
}

void GameModel::changeScene(Scene scene)
{
    scene_ = scene;
    bus_.publish(events::SceneChanged{scene_});
}

void GameModel::publishMenuState() const
{
    events::MenuStateChanged state{{}, selectedIndex_};
    state.labels.reserve(menuItems_.size());
    for (const auto& item : menuItems_) {
        state.labels.push_back(item.label);
    }
    bus_.publish(state);
}

void GameModel::resetWorld()
{
    world_->clear();
    bus_.publish(events::WorldReset{});
    buildings_->reset();
}

void GameModel::startMatch()
{
    resetWorld();
    changeScene(Scene::Playing);
}

} // namespace model
