#pragma once

#include "core/EventBus.hpp"
#include "model/build/BuildingSystem.hpp"
#include "model/GridPos.hpp"
#include "model/player/Player.hpp"
#include "model/units/UnitType.hpp"
#include "model/units/UnitComponents.hpp"
#include "model/ecs/Registry.hpp"

#include <functional>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace model {

// Unit rules: recruiting, spawning, move orders (formation + A*) and walking along paths.
// A unit of size N covers an N x N square of tiles and reserves the square it stands on (or is
// heading to), so idle units never overlap. It only walks where its whole square fits.
class UnitSystem {
public:
    UnitSystem(core::EventBus& bus, ecs::Registry& world, const BuildingSystem& buildings);

    // Call after the world is cleared.
    void reset();
    void setTileWalkable(std::function<bool(GridPos)> predicate) { tileWalkable_ = std::move(predicate); }

    // Spawns next to the Command Center, on the nearest free tile.
    void recruit(PlayerId player, UnitType type, ecs::Entity commandCenter);
    // Spawns a non-combat drone next to the supplied tile.
    [[nodiscard]] std::optional<ecs::Entity> spawnTransportDrone(PlayerId player, GridPos center);
    [[nodiscard]] bool moveToAdjacent(ecs::Entity entity, GridPos center);
    // Walks the unit to the nearest free square from which a target centered on `target` is
    // within `range` tiles. Does nothing (and returns true) when already in range.
    [[nodiscard]] bool moveWithinRange(ecs::Entity entity, GridPos target, float range);
    void orderMove(PlayerId player, const std::vector<ecs::Entity>& units, GridPos target);
    // Clears the current path and settles the unit on the nearest free square.
    void stop(ecs::Entity entity);

    // Returns true when any unit moved.
    bool update(float deltaSeconds);

    // A unit covers or reserved the tile, so nothing may be built there.
    [[nodiscard]] bool isTileTaken(GridPos tile) const;

    ecs::Entity spawn(PlayerId player, const UnitStats& stats, GridPos origin);
    void remove(ecs::Entity entity);

    // Origin of the free square of `size` nearest to `center`, at most `radius` rings away.
    // Tiles covered or reserved by the `ignored` units count as free.
    [[nodiscard]] std::optional<GridPos> findSpawnSquare(
        GridPos center, int size, const std::vector<ecs::Entity>& ignored, int radius) const;

private:
    struct Walker {
        ecs::Entity entity;
        components::Mover* mover;
        components::Position* position;
        int size;
    };

    [[nodiscard]] int sizeOf(ecs::Entity entity) const;
    [[nodiscard]] bool isSquareWalkable(GridPos origin, int size) const;
    [[nodiscard]] bool isSquareFreeDestination(GridPos origin, int size) const;

    bool startPath(const Walker& walker, GridPos destination);
    // Stops the unit on the nearest free square around it.
    void settle(const Walker& walker);
    void reserve(ecs::Entity entity, GridPos origin, int size);
    void release(const Walker& walker);
    bool advance(const Walker& walker, float deltaSeconds);

    core::EventBus& bus_;
    ecs::Registry& world_;
    const BuildingSystem& buildings_;
    std::unordered_map<GridPos, ecs::Entity, GridPosHash> reserved_;
    std::function<bool(GridPos)> tileWalkable_;
};

} // namespace model
