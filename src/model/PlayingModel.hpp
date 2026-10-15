#pragma once

#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/BuildingSystem.hpp"
#include "model/MergeSystem.hpp"
#include "model/SelectionSystem.hpp"
#include "model/Player.hpp"
#include "model/Scene.hpp"
#include "model/UnitSystem.hpp"
#include "model/ecs/Registry.hpp"

#include <memory>
#include <string>

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

    core::EventBus& bus_;
    Scene scene_ = Scene::MainMenu;
    PlayerId nextPlayerId_ = 1;
    PlayerId localPlayerId_ = kNoPlayer;
    std::unique_ptr<ecs::Registry> world_;
    // Systems hold references to world_ (and units_ to buildings_); preserve declaration order.
    std::unique_ptr<BuildingSystem> buildings_;
    std::unique_ptr<UnitSystem> units_;
    std::unique_ptr<SelectionSystem> selection_;
    std::unique_ptr<MergeSystem> merges_; // References units_ and selection_.
};

} // namespace model
