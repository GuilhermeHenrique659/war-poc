#pragma once

#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/GridPos.hpp"
#include "model/player/Player.hpp"
#include "model/units/UnitComponents.hpp"
#include "model/ecs/Entity.hpp"
#include "model/ecs/Registry.hpp"

#include <string>
#include <optional>
#include <unordered_map>

namespace model {

class PlayerAvatarSystem {
public:
    PlayerAvatarSystem(core::EventBus& bus, ecs::Registry& world);

    void reset();
    [[nodiscard]]     ecs::Entity create(PlayerId player, const std::string& name);
    [[nodiscard]] std::optional<ecs::Entity> entity(PlayerId player) const;
    void move(PlayerId player, float directionX, float directionY, float deltaSeconds);

private:
    void onBuildingPlaced(const events::BuildingPlaced& event);
    void respawn(ecs::Entity entity);
    [[nodiscard]] components::Position findSpawnPosition(GridPos origin, int size) const;
    [[nodiscard]] bool collidesWithBuilding(components::Position position) const;
    [[nodiscard]] float moveAlongAxis(components::Position position, float distance, bool xAxis) const;

    core::EventBus& bus_;
    ecs::Registry& world_;
    std::unordered_map<PlayerId, ecs::Entity> entities_;
    std::unordered_map<ecs::Entity, components::Position> spawnPositions_;
};

} // namespace model
