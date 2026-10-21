#include "model/PlayingModel.hpp"

#include "model/build/BuildingComponents.hpp"
#include "model/components/Owner.hpp"
#include "model/economy/InventoryComponents.hpp"
#include "model/economy/InventorySystem.hpp"
#include "model/player/PlayerComponents.hpp"

namespace model {

PlayingModel::PlayingModel(core::EventBus& bus)
    : bus_(bus)
    , world_(std::make_unique<ecs::Registry>())
    , buildings_(std::make_unique<BuildingSystem>(bus_, *world_))
    , units_(std::make_unique<UnitSystem>(bus_, *world_, *buildings_))
    , playerAvatars_(std::make_unique<PlayerAvatarSystem>(bus_, *world_))
    , combat_(std::make_unique<CombatSystem>(bus_, *world_, *units_, *buildings_))
    , selection_(std::make_unique<SelectionSystem>(bus_, *world_))
    , genealogy_(std::make_unique<GenealogySystem>(*world_, *units_, *selection_))
{
    buildings_->setTileBlocker([this](GridPos tile) { return units_->isTileTaken(tile); });

    bus_.subscribe<events::SceneChanged>([this](const auto& e) { scene_ = e.scene; });
    bus_.subscribe<events::StartMatchRequested>([this](const auto& e) { onStartMatch(e); });
    bus_.subscribe<events::ReturnToMenuRequested>([this](const auto& e) { onReturnToMenu(e); });
    bus_.subscribe<events::SelectionChanged>([this](const events::SelectionChanged& event) {
        if (!inMatch()) {
            return;
        }
        if (event.building && world_->has<components::Inventory>(*event.building)) {
            const auto* owner = world_->tryGet<components::Owner>(*event.building);
            if (owner != nullptr && owner->player == event.player) {
                inventoryPanels_[event.player] = *event.building;
                publishInventoryPanel(event.player);
                return;
            }
        }
        inventoryPanels_.erase(event.player);
        publishInventoryPanel(event.player);
    });
    bus_.subscribe<events::TogglePlayerInventoryRequested>([this](const events::TogglePlayerInventoryRequested& e) {
        if (!inMatch()) {
            return;
        }
        playerInventoryOpen_[e.player] = !playerInventoryOpen_[e.player];
        publishInventoryPanel(e.player);
    });
    bus_.subscribe<events::CloseInventoryRequested>([this](const events::CloseInventoryRequested& e) {
        if (!inMatch()) {
            return;
        }
        playerInventoryOpen_[e.player] = false;
        inventoryPanels_.erase(e.player);
        publishInventoryPanel(e.player);
    });
    bus_.subscribe<events::TransferInventoryItemRequested>([this](const events::TransferInventoryItemRequested& e) {
        const auto panel = inventoryPanels_.find(e.player);
        const auto playerEntity = playerAvatars_->entity(e.player);
        if (!inMatch() || panel == inventoryPanels_.end() || panel->second != e.building || !playerEntity) {
            return;
        }
        const auto* owner = world_->tryGet<components::Owner>(e.building);
        auto* buildingInventory = world_->tryGet<components::Inventory>(e.building);
        auto* playerInventory = world_->tryGet<components::Inventory>(*playerEntity);
        if (owner == nullptr || owner->player != e.player || buildingInventory == nullptr || playerInventory == nullptr
            || e.slotIndex >= buildingInventory->slots.size()) {
            return;
        }
        const auto& sourceSlots = e.fromPlayer ? playerInventory->slots : buildingInventory->slots;
        if (e.slotIndex >= sourceSlots.size()) {
            return;
        }
        const auto& slot = sourceSlots[e.slotIndex];
        if (!slot) {
            return;
        }
        if (e.fromPlayer) {
            (void)InventorySystem::transfer(
                *playerInventory, *buildingInventory, slot->item, slot->currentQuantity);
        } else {
            (void)InventorySystem::transfer(
                *buildingInventory, *playerInventory, slot->item, slot->currentQuantity);
        }
        publishInventoryPanel(e.player);
        buildings_->refreshPreviews();
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
    production_.update(*world_, deltaSeconds);
    for (const auto& [player, building] : inventoryPanels_) {
        (void)building;
        publishInventoryPanel(player);
    }
    // Moving units can make a placement preview (in)valid.
    if (units_->update(deltaSeconds)) {
        buildings_->refreshPreviews();
    }
    combat_->update(deltaSeconds);
    genealogy_->update();
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
    inventoryPanels_.clear();
    playerInventoryOpen_.clear();
    production_.reset();
    bus_.publish(events::WorldReset{});
    buildings_->reset();
    units_->reset();
    playerAvatars_->reset();
    combat_->reset();
    selection_->reset();
    genealogy_->reset();
}

void PlayingModel::createLocalPlayer(const std::string& playerName)
{
    localPlayerId_ = nextPlayerId_++;
    bus_.publish(events::LocalPlayerChanged{localPlayerId_});
    (void)playerAvatars_->create(localPlayerId_, playerName);
    buildings_->initializePlayer(localPlayerId_);
    selection_->initializePlayer(localPlayerId_);
}

void PlayingModel::publishInventoryPanel(PlayerId player) const
{
    const auto panel = inventoryPanels_.find(player);
    const auto playerEntity = playerAvatars_->entity(player);
    if (!playerEntity) {
        events::InventoryPanelChanged closed{};
        closed.player = player;
        bus_.publish(closed);
        return;
    }
    const auto* playerInventory = world_->tryGet<components::Inventory>(*playerEntity);
    if (playerInventory == nullptr) {
        events::InventoryPanelChanged closed{};
        closed.player = player;
        bus_.publish(closed);
        return;
    }

    const auto buildingEntity = panel == inventoryPanels_.end()
        ? std::optional<ecs::Entity>{}
        : std::optional<ecs::Entity>{panel->second};
    const auto* building = buildingEntity ? world_->tryGet<components::Building>(*buildingEntity) : nullptr;
    const auto* buildingInventory
        = buildingEntity ? world_->tryGet<components::Inventory>(*buildingEntity) : nullptr;
    const bool hasBuilding = building != nullptr && buildingInventory != nullptr;
    const auto isOpen = playerInventoryOpen_.find(player);
    const bool playerInventoryOpen = isOpen != playerInventoryOpen_.end() && isOpen->second;
    if (!hasBuilding && !playerInventoryOpen) {
        events::InventoryPanelChanged closed{};
        closed.player = player;
        bus_.publish(closed);
        return;
    }

    const auto makeSlots = [](const components::Inventory& inventory) {
        std::vector<events::InventorySlotState> slots;
        slots.reserve(inventory.slots.size());
        for (const auto& slot : inventory.slots) {
            if (!slot) {
                slots.emplace_back();
                continue;
            }
            slots.push_back(events::InventorySlotState{
                true, slot->item, slot->currentQuantity, slot->maxQuantity});
        }
        return slots;
    };

    events::InventoryPanelChanged event{};
    event.player = player;
    event.open = true;
    event.playerVisible = true;
    event.hasBuilding = hasBuilding;
    event.playerInventoryOpen = playerInventoryOpen;
    if (hasBuilding) {
        event.building = *buildingEntity;
        event.buildingName = building->name;
        event.buildingSlots = makeSlots(*buildingInventory);
    }
    event.playerSlots = makeSlots(*playerInventory);
    bus_.publish(event);
}

} // namespace model
