#pragma once

#include "core/EventBus.hpp"
#include "model/BuildingSystem.hpp"
#include "model/GridPos.hpp"
#include "model/Player.hpp"
#include "model/UnitType.hpp"
#include "model/components/UnitComponents.hpp"
#include "model/ecs/Registry.hpp"

#include <unordered_map>
#include <vector>

namespace model {

// Unit rules: recruiting, move orders (formation + A*) and walking along paths.
// Each unit reserves one tile (its destination) so idle units never share a tile.
class UnitSystem {
public:
    UnitSystem(core::EventBus& bus, ecs::Registry& world, const BuildingSystem& buildings);

    // Call after the world is cleared.
    void reset();

    // Spawns next to the Command Center, on the nearest free tile.
    void recruit(PlayerId player, UnitType type, ecs::Entity commandCenter);
    void orderMove(PlayerId player, const std::vector<ecs::Entity>& units, GridPos target);

    // Returns true when any unit moved.
    bool update(float deltaSeconds);

    // A unit stands on or reserved the tile, so nothing may be built there.
    [[nodiscard]] bool isTileTaken(GridPos tile) const;

private:
    [[nodiscard]] bool isWalkable(GridPos tile) const;
    [[nodiscard]] bool isFreeDestination(GridPos tile) const;

    bool startPath(ecs::Entity entity, components::Mover& mover, const components::Position& position, GridPos destination);
    // Stops the unit on the nearest free tile around it.
    void settle(ecs::Entity entity, components::Mover& mover, const components::Position& position);
    void release(ecs::Entity entity, const components::Mover& mover);
    bool advance(ecs::Entity entity, components::Mover& mover, components::Position& position, float deltaSeconds);

    core::EventBus& bus_;
    ecs::Registry& world_;
    const BuildingSystem& buildings_;
    std::unordered_map<GridPos, ecs::Entity, GridPosHash> reserved_;
};

} // namespace model
