#pragma once

#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/build/BuildingType.hpp"
#include "model/ecs/Entity.hpp"
#include "model/GridPos.hpp"
#include "model/player/Player.hpp"
#include "model/units/UnitType.hpp"
#include "view/world/WorldEntityViewModels.hpp"

#include <raylib.h>

#include <optional>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace view::gameplay {

class WorldObjectsViewModel {
public:
    explicit WorldObjectsViewModel(core::EventBus& bus);

    void draw() const;
    [[nodiscard]] std::optional<Vector2> playerGridPosition(model::PlayerId player) const;

private:
    struct BuildingVisual {
        model::ecs::Entity entity;
        model::GridPos origin;
        int size;
        std::unique_ptr<BuildingViewModel> viewModel;
    };

    struct UnitVisual {
        model::ecs::Entity entity;
        float x;
        float y;
        int size;
        std::unique_ptr<UnitViewModel> viewModel;
    };

    struct DroneVisual {
        model::ecs::Entity entity;
        float x;
        float y;
        std::unique_ptr<DroneViewModel> viewModel;
    };

    struct PlayerVisual {
        model::ecs::Entity entity;
        model::PlayerId player;
        float x;
        float y;
        std::unique_ptr<PlayerAvatarViewModel> viewModel;
    };

    void subscribeUnits();
    void subscribePlayers();
    void onSelectionChanged(const events::SelectionChanged& event);

    core::EventBus& bus_;
    std::vector<BuildingVisual> buildings_;
    std::unordered_map<model::ecs::Entity, UnitVisual> units_;
    std::unordered_map<model::ecs::Entity, DroneVisual> drones_;
    std::unordered_map<model::ecs::Entity, PlayerVisual> players_;
    std::unordered_set<model::ecs::Entity> selectedUnits_;
    std::optional<model::ecs::Entity> selectedBuilding_;
    model::PlayerId localPlayerId_ = model::kNoPlayer;
};

} // namespace view::gameplay
