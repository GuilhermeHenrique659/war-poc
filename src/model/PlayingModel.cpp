#include "model/PlayingModel.hpp"

#include "model/components/PlayerComponents.hpp"

namespace model {

PlayingModel::PlayingModel(core::EventBus& bus)
    : bus_(bus)
    , world_(std::make_unique<ecs::Registry>())
    , buildings_(std::make_unique<BuildingSystem>(bus_, *world_))
    , units_(std::make_unique<UnitSystem>(bus_, *world_, *buildings_))
    , selection_(std::make_unique<SelectionSystem>(bus_, *world_))
    , merges_(std::make_unique<MergeSystem>(bus_, *world_, *units_, *selection_))
{
    buildings_->setTileBlocker([this](GridPos tile) { return units_->isTileTaken(tile); });

    bus_.subscribe<events::SceneChanged>([this](const auto& e) { scene_ = e.scene; });
    bus_.subscribe<events::StartMatchRequested>([this](const auto& e) { onStartMatch(e); });
    bus_.subscribe<events::ReturnToMenuRequested>([this](const auto& e) { onReturnToMenu(e); });
    subscribeBuildCommands();
    subscribeUnitCommands();
}

void PlayingModel::subscribeUnitCommands()
{
    bus_.subscribe<events::RecruitRequested>([this](const events::RecruitRequested& e) {
        if (!inMatch()) {
            return;
        }
        if (const auto commandCenter = selection_->selectedCommandCenter(e.player)) {
            units_->recruit(e.player, e.type, *commandCenter);
        }
    });
    bus_.subscribe<events::SelectUnitsRequested>([this](const events::SelectUnitsRequested& e) {
        if (inMatch()) {
            selection_->selectUnits(e.player, e.units, e.additive);
            merges_->refresh(e.player);
        }
    });
    bus_.subscribe<events::SelectBuildingRequested>([this](const events::SelectBuildingRequested& e) {
        if (inMatch()) {
            selection_->selectBuilding(e.player, e.building);
            merges_->refresh(e.player);
        }
    });
    bus_.subscribe<events::SelectionClearRequested>([this](const events::SelectionClearRequested& e) {
        if (inMatch()) {
            selection_->clear(e.player);
            merges_->refresh(e.player);
        }
    });
    bus_.subscribe<events::MoveOrderRequested>([this](const events::MoveOrderRequested& e) {
        if (inMatch()) {
            units_->orderMove(e.player, selection_->selectedUnits(e.player), e.tile);
        }
    });
    bus_.subscribe<events::MergeRequested>([this](const events::MergeRequested& e) {
        if (inMatch()) {
            merges_->merge(e.player);
        }
    });
}

void PlayingModel::subscribeBuildCommands()
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

void PlayingModel::update(float deltaSeconds)
{
    if (!inMatch()) {
        return;
    }
    // Moving units can make a placement preview (in)valid.
    if (units_->update(deltaSeconds)) {
        buildings_->refreshPreviews();
    }
    // Merge validity depends on positions and free space; it only republishes on change.
    merges_->refreshAll();
}

void PlayingModel::onStartMatch(const events::StartMatchRequested& event)
{
    resetWorld();
    createLocalPlayer(event.playerName);
    changeScene(Scene::Playing);
}

void PlayingModel::onReturnToMenu(const events::ReturnToMenuRequested& /*event*/)
{
    if (!inMatch()) {
        return;
    }
    resetWorld();
    changeScene(Scene::MainMenu);
}

void PlayingModel::changeScene(Scene scene)
{
    scene_ = scene;
    bus_.publish(events::SceneChanged{scene_});
}

void PlayingModel::resetWorld()
{
    world_->clear();
    bus_.publish(events::WorldReset{});
    buildings_->reset();
    units_->reset();
    selection_->reset();
    merges_->reset();
}

void PlayingModel::createLocalPlayer(const std::string& playerName)
{
    const ecs::Entity entity = world_->create();
    localPlayerId_ = nextPlayerId_++;
    world_->emplace<components::PlayerProfile>(entity, localPlayerId_, playerName);
    bus_.publish(events::LocalPlayerChanged{localPlayerId_});
    buildings_->initializePlayer(localPlayerId_);
    selection_->initializePlayer(localPlayerId_);
    merges_->initializePlayer(localPlayerId_);
}

} // namespace model
