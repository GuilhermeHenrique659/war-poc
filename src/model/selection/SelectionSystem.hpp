#pragma once

#include "core/EventBus.hpp"
#include "model/player/Player.hpp"
#include "model/ecs/Registry.hpp"

#include <optional>
#include <unordered_map>
#include <vector>

namespace model {

// What each player has selected: either a group of their units or one of their buildings.
// Selecting their Command Center opens the recruit panel.
class SelectionSystem {
public:
    SelectionSystem(core::EventBus& bus, ecs::Registry& world);

    // Call after the world is cleared.
    void reset();
    void initializePlayer(PlayerId player);

    void selectUnits(PlayerId player, const std::vector<ecs::Entity>& units, bool additive);
    void selectBuilding(PlayerId player, ecs::Entity building);
    void clear(PlayerId player);

    [[nodiscard]] std::vector<ecs::Entity> selectedUnits(PlayerId player) const;
    [[nodiscard]] std::optional<ecs::Entity> selectedCommandCenter(PlayerId player) const;

private:
    struct PlayerSelection {
        std::vector<ecs::Entity> units;
        std::optional<ecs::Entity> building;
    };

    [[nodiscard]] bool isOwnedBy(ecs::Entity entity, PlayerId player) const;
    void removeEntity(ecs::Entity entity);
    void publish(PlayerId player) const;

    core::EventBus& bus_;
    ecs::Registry& world_;
    std::unordered_map<PlayerId, PlayerSelection> selections_;
};

} // namespace model
