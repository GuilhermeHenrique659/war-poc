#pragma once

#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/build/BuildingSystem.hpp"
#include "model/economy/InventoryPanelSystem.hpp"
#include "model/economy/LogisticsSystem.hpp"
#include "model/economy/ProductionSystem.hpp"
#include "model/units/CombatSystem.hpp"
#include "model/units/GenealogySystem.hpp"
#include "model/selection/SelectionSystem.hpp"
#include "model/player/Player.hpp"
#include "model/player/PlayerAvatarSystem.hpp"
#include "model/Scene.hpp"
#include "model/WorldMap.hpp"
#include "model/units/UnitSupplySystem.hpp"
#include "model/units/UnitSystem.hpp"
#include "model/ecs/Registry.hpp"

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

namespace model {

// Owns the active match world and its gameplay systems.
class PlayingModel {
public:
    explicit PlayingModel(core::EventBus& bus);

    void update(float deltaSeconds);

private:
    void subscribeBuildCommands();
    void subscribeUnitCommands();
    [[nodiscard]] bool inMatch() const { return scene_ == Scene::Playing; }
    void onStartMatch(const events::StartMatchRequested& event);
    void onReturnToMenu(const events::ReturnToMenuRequested& event);
    void changeScene(Scene scene);
    void resetWorld();
    void createLocalPlayer(const std::string& playerName);
    void publishLogisticsPanel(PlayerId player) const;

    core::EventBus& bus_;
    Scene scene_ = Scene::MainMenu;
    PlayerId nextPlayerId_ = 1;
    PlayerId localPlayerId_ = kNoPlayer;
    std::optional<WorldMap> worldMap_;
    std::unique_ptr<ecs::Registry> world_;
    // Systems hold references to world_ (and units_ to buildings_); preserve declaration order.
    std::unique_ptr<BuildingSystem> buildings_;
    std::unique_ptr<UnitSystem> units_;
    std::unique_ptr<UnitSupplySystem> unitSupplies_;
    std::unique_ptr<LogisticsSystem> logistics_;
    std::unique_ptr<PlayerAvatarSystem> playerAvatars_;
    std::unique_ptr<CombatSystem> combat_;
    std::unique_ptr<SelectionSystem> selection_;
    std::unique_ptr<GenealogySystem> genealogy_; // References buildings_, units_ and selection_.
    std::unique_ptr<InventoryPanelSystem> inventoryPanels_;
    ProductionSystem production_;
    std::unordered_map<PlayerId, ecs::Entity> logisticsPanels_;
};

} // namespace model
