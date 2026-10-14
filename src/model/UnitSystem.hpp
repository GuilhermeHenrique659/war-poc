#pragma once

#include "core/EventBus.hpp"
#include "model/BuildingSystem.hpp"
#include "model/GridPos.hpp"
#include "model/Player.hpp"
#include "model/UnitType.hpp"
#include "model/components/UnitComponents.hpp"
#include "model/ecs/Registry.hpp"

#include <optional>
#include <unordered_map>
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

    // Spawns next to the Command Center, on the nearest free tile.
    void recruit(PlayerId player, UnitType type, ecs::Entity commandCenter);
    void orderMove(PlayerId player, const std::vector<ecs::Entity>& units, GridPos target);

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
};

} // namespace model
