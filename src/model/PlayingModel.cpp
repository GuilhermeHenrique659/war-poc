#include "model/PlayingModel.hpp"

#include "model/build/BuildingComponents.hpp"
#include "model/build/ConstructionDroneSystem.hpp"
#include "model/build/ConstructionSystem.hpp"
#include "model/components/Owner.hpp"
#include "model/economy/LogisticsComponents.hpp"

#include <random>

namespace model {

PlayingModel::PlayingModel(core::EventBus& bus)
    : bus_(bus)
    , time_(bus)
    , world_(std::make_unique<ecs::Registry>())
    , economy_(std::make_unique<EconomySystem>(bus_))
    , trade_(std::make_unique<TradeSystem>(bus_, *world_, *economy_))
    , buildings_(std::make_unique<BuildingSystem>(bus_, *world_))
    , units_(std::make_unique<UnitSystem>(bus_, *world_, *buildings_))
    , construction_(std::make_unique<ConstructionSystem>(bus_, *world_))
    , constructionDrones_(std::make_unique<ConstructionDroneSystem>(bus_, *world_, *units_))
    , unitSupplies_(std::make_unique<UnitSupplySystem>(*world_, *units_))
    , logistics_(std::make_unique<LogisticsSystem>(bus_, *world_, *units_))
    , playerAvatars_(std::make_unique<PlayerAvatarSystem>(bus_, *world_))
    , damage_(std::make_unique<DamageSystem>(bus_, *world_, *units_, *buildings_))
    , projectiles_(std::make_unique<ProjectileSystem>(bus_, *world_, *damage_))
    , combat_(std::make_unique<CombatSystem>(bus_, *world_, *units_, *buildings_, *damage_, *projectiles_))
    , selection_(std::make_unique<SelectionSystem>(bus_, *world_))
    , genealogy_(std::make_unique<GenealogySystem>(*world_, *units_, *selection_))
    , inventoryPanels_(std::make_unique<InventoryPanelSystem>(bus_, *world_, *selection_, *playerAvatars_))
    , production_(bus_)
{
    buildings_->setTileBlocker([this](GridPos tile) { return units_->isTileTaken(tile); });
    buildings_->setPlacementRule([this](BuildingType type, GridPos origin, int size) {
        return !worldMap_ || worldMap_->canPlaceBuilding(type, origin, size);
    });
    units_->setTileWalkable([this](GridPos tile) { return !worldMap_ || worldMap_->isWalkable(tile); });
    playerAvatars_->setTileWalkable([this](GridPos tile) { return !worldMap_ || worldMap_->isWalkable(tile); });
    combat_->setTileSightBlocker([this](GridPos tile) {
        return buildings_->isOccupied(tile)
            || (worldMap_ && worldMap_->terrain().terrainAt(tile) == Terrain::Blocked);
    });

    bus_.subscribe<events::SceneChanged>([this](const auto& e) { scene_ = e.scene; });
    bus_.subscribe<events::StartMatchRequested>([this](const auto& e) { onStartMatch(e); });
    bus_.subscribe<events::ReturnToMenuRequested>([this](const auto& e) { onReturnToMenu(e); });
    bus_.subscribe<events::SelectionChanged>([this](const events::SelectionChanged& event) {
        if (!inMatch()) {
            return;
        }
        const auto* owner = event.building ? world_->tryGet<components::Owner>(*event.building) : nullptr;
        if (event.building && owner != nullptr && owner->player == event.player
            && logistics_->supportsBuilding(*event.building)) {
            logisticsPanels_[event.player] = *event.building;
        } else {
            logisticsPanels_.erase(event.player);
        }
        publishLogisticsPanel(event.player);

        const auto* selectedBuilding =
            event.building ? world_->tryGet<components::Building>(*event.building) : nullptr;
        if (event.building && owner != nullptr && owner->player == event.player && selectedBuilding != nullptr
            && selectedBuilding->type == BuildingType::TradeCenter) {
            tradePanels_[event.player] = *event.building;
        } else {
            tradePanels_.erase(event.player);
        }
        publishTradePanel(event.player);
    });
    bus_.subscribe<events::TransferInventoryItemRequested>([this](const events::TransferInventoryItemRequested& e) {
        if (inMatch() && inventoryPanels_->transfer(e.player, e.building, e.slotIndex, e.fromPlayer)) {
            buildings_->refreshPreviews();
        }
    });
    bus_.subscribe<events::RecruitDroneRequested>([this](const events::RecruitDroneRequested& e) {
        if (!inMatch()) {
            return;
        }
        (void)logistics_->recruitDrone(e.player, e.warehouse);
        publishLogisticsPanel(e.player);
    });
    bus_.subscribe<events::RecruitConstructionDroneRequested>(
        [this](const events::RecruitConstructionDroneRequested& e) {
            if (inMatch()) {
                (void)constructionDrones_->recruit(e.player);
            }
        });
    bus_.subscribe<events::AssignDroneToRouteRequested>([this](const events::AssignDroneToRouteRequested& e) {
        if (!inMatch()) {
            return;
        }
        (void)logistics_->assignIdleDrone(e.player, e.source, e.routeId);
        publishLogisticsPanel(e.player);
    });
    bus_.subscribe<events::UnassignDroneFromRouteRequested>([this](const events::UnassignDroneFromRouteRequested& e) {
        if (!inMatch()) {
            return;
        }
        (void)logistics_->unassignDrone(e.player, e.source, e.routeId);
        publishLogisticsPanel(e.player);
    });
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
        }
    });
    bus_.subscribe<events::SelectBuildingRequested>([this](const events::SelectBuildingRequested& e) {
        if (inMatch()) {
            selection_->selectBuilding(e.player, e.building);
        }
    });
    bus_.subscribe<events::SelectionClearRequested>([this](const events::SelectionClearRequested& e) {
        if (inMatch()) {
            selection_->clear(e.player);
        }
    });
    bus_.subscribe<events::MoveOrderRequested>([this](const events::MoveOrderRequested& e) {
        if (inMatch()) {
            units_->orderMove(e.player, selection_->selectedUnits(e.player), e.tile);
        }
    });
    bus_.subscribe<events::CruzadorOrderRequested>([this](const events::CruzadorOrderRequested& e) {
        if (inMatch()) {
            (void)genealogy_->send(e.player, e.cruzador, e.units);
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
    time_.update(deltaSeconds);
    production_.update(*world_, deltaSeconds);
    construction_->update(deltaSeconds);
    inventoryPanels_->update();
    // Resupply decisions happen before movement, so a unit can start walking in the same frame.
    unitSupplies_->update();
    // Moving units can make a placement preview (in)valid.
    if (units_->update(deltaSeconds)) {
        buildings_->refreshPreviews();
    }
    logistics_->update();
    constructionDrones_->update();
    trade_->update();
    for (const auto& [player, building] : logisticsPanels_) {
        (void)building;
        publishLogisticsPanel(player);
    }
    for (const auto& [player, building] : tradePanels_) {
        (void)building;
        publishTradePanel(player);
    }
    combat_->update(deltaSeconds);
    projectiles_->update(deltaSeconds);
    genealogy_->update();
}

void PlayingModel::onStartMatch(const events::StartMatchRequested& event)
{
    resetWorld();
    std::random_device entropy;
    const std::uint32_t seed = entropy();
    worldMap_.emplace(generateWorldMap(seed));
    const BoundedMap& terrain = worldMap_->terrain();
    bus_.publish(events::MapGenerated{
        seed, terrain.width(), terrain.height(), terrain.origin(), terrain.tiles(), worldMap_->deposits()});
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
    worldMap_.reset();
    logisticsPanels_.clear();
    tradePanels_.clear();
    inventoryPanels_->reset();
    production_.reset();
    economy_->reset();
    trade_->reset();
    logistics_->reset();
    construction_->reset();
    constructionDrones_->reset();
    bus_.publish(events::WorldReset{});
    buildings_->reset();
    units_->reset();
    playerAvatars_->reset();
    combat_->reset();
    projectiles_->reset();
    selection_->reset();
    genealogy_->reset();
    time_.reset();
}

void PlayingModel::createLocalPlayer(const std::string& playerName)
{
    localPlayerId_ = nextPlayerId_++;
    bus_.publish(events::LocalPlayerChanged{localPlayerId_});
    economy_->initializePlayer(localPlayerId_);
    (void)playerAvatars_->create(localPlayerId_, playerName);
    buildings_->initializePlayer(localPlayerId_);
    selection_->initializePlayer(localPlayerId_);
}

void PlayingModel::publishLogisticsPanel(PlayerId player) const
{
    const auto panel = logisticsPanels_.find(player);
    if (panel == logisticsPanels_.end()) {
        events::LogisticsPanelChanged closed{};
        closed.player = player;
        bus_.publish(closed);
        return;
    }
    const ecs::Entity buildingEntity = panel->second;
    const auto* building = world_->tryGet<components::Building>(buildingEntity);
    const auto* owner = world_->tryGet<components::Owner>(buildingEntity);
    if (building == nullptr || owner == nullptr || owner->player != player
        || !logistics_->supportsBuilding(buildingEntity)) {
        events::LogisticsPanelChanged closed{};
        closed.player = player;
        bus_.publish(closed);
        return;
    }

    events::LogisticsPanelChanged event{};
    event.player = player;
    event.open = true;
    event.building = buildingEntity;
    event.buildingType = building->type;
    event.free = logistics_->debugMode();
    event.warehouse = building->type == BuildingType::Warehouse;
    event.idleDrones = logistics_->idleDroneCount(player);
    if (const auto* routes = world_->tryGet<components::LogisticsRoutes>(buildingEntity)) {
        for (const components::LogisticsRoute& route : routes->routes) {
            const auto* destination = world_->tryGet<components::Building>(route.destination);
            if (destination == nullptr) {
                continue;
            }
            event.routes.push_back(events::LogisticsRouteEntry{
                route.id, route.destination, destination->type, route.item, route.assignedDrones});
        }
    }
    bus_.publish(event);
}

void PlayingModel::publishTradePanel(PlayerId player) const
{
    const auto panel = tradePanels_.find(player);
    if (panel == tradePanels_.end()) {
        trade_->closePanel(player);
        return;
    }
    trade_->publishPanel(player, panel->second);
}

} // namespace model
