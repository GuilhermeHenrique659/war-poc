#include "model/UnitSystem.hpp"

#include "core/Events.hpp"
#include "model/Pathfinding.hpp"
#include "model/components/BuildingComponents.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <utility>

namespace model {

namespace {

// Keeps a unit standing exactly on its square from touching the neighbor tiles.
constexpr float kCoverEpsilon = 0.01f;

[[nodiscard]] float halfSize(int size)
{
    return static_cast<float>(size) / 2.0f;
}

[[nodiscard]] GridPos originOf(const components::Position& position, int size)
{
    const float offset = halfSize(size) - 0.5f;
    return GridPos{static_cast<int>(std::floor(position.x - offset)), static_cast<int>(std::floor(position.y - offset))};
}

[[nodiscard]] components::Position centerOf(GridPos origin, int size)
{
    return components::Position{static_cast<float>(origin.x) + halfSize(size), static_cast<float>(origin.y) + halfSize(size)};
}

[[nodiscard]] bool covers(const components::Position& position, int size, GridPos tile)
{
    const float half = halfSize(size) - kCoverEpsilon;
    const float x = static_cast<float>(tile.x);
    const float y = static_cast<float>(tile.y);
    return x + 1.0f > position.x - half && x < position.x + half && y + 1.0f > position.y - half
        && y < position.y + half;
}

template <typename Func>
bool anyTileOf(GridPos origin, int size, Func&& func)
{
    for (int dy = 0; dy < size; ++dy) {
        for (int dx = 0; dx < size; ++dx) {
            if (func(GridPos{origin.x + dx, origin.y + dy})) {
                return true;
            }
        }
    }
    return false;
}

// Origin that centers a square of `size` on the tile.
[[nodiscard]] GridPos centeredOrigin(GridPos tile, int size)
{
    return GridPos{tile.x - (size - 1) / 2, tile.y - (size - 1) / 2};
}

} // namespace

UnitSystem::UnitSystem(core::EventBus& bus, ecs::Registry& world, const BuildingSystem& buildings)
    : bus_(bus)
    , world_(world)
    , buildings_(buildings)
{
}

void UnitSystem::reset()
{
    reserved_.clear();
}

void UnitSystem::recruit(PlayerId player, UnitType type, ecs::Entity commandCenter)
{
    const auto* footprint = world_.tryGet<components::Footprint>(commandCenter);
    if (footprint == nullptr) {
        return;
    }

    const UnitStats stats = recruitStats(type);
    const GridPos center{footprint->origin.x + footprint->size / 2, footprint->origin.y + footprint->size / 2};
    const auto origin = findSpawnSquare(center, stats.size, {}, pathfinding::kUnlimitedRadius);
    if (!origin) {
        return;
    }
    spawn(player, stats, *origin);
}

ecs::Entity UnitSystem::spawn(PlayerId player, const UnitStats& stats, GridPos origin)
{
    const components::Position position = centerOf(origin, stats.size);
    components::Mover mover{stats.speed, {}, 0, origin};

    const ecs::Entity entity = world_.create();
    world_.emplace<components::Unit>(entity, stats.type, stats.baseCount, stats.size);
    world_.emplace<components::Owner>(entity, player);
    world_.emplace<components::Health>(entity, stats.health, stats.maxHealth);
    world_.emplace<components::Combat>(entity, stats.damage, stats.range);
    world_.emplace<components::Position>(entity, position);
    world_.emplace<components::Mover>(entity, std::move(mover));
    reserve(entity, origin, stats.size);

    bus_.publish(events::UnitSpawned{entity, player, stats.type, position.x, position.y, stats.health,
        stats.maxHealth, stats.size, stats.baseCount});
    return entity;
}

void UnitSystem::remove(ecs::Entity entity)
{
    if (!world_.isAlive(entity)) {
        return;
    }
    auto* mover = world_.tryGet<components::Mover>(entity);
    auto* position = world_.tryGet<components::Position>(entity);
    if (mover != nullptr && position != nullptr) {
        release(Walker{entity, mover, position, sizeOf(entity)});
    }
    world_.destroy(entity);
    bus_.publish(events::UnitRemoved{entity});
}

void UnitSystem::orderMove(PlayerId player, const std::vector<ecs::Entity>& units, GridPos target)
{
    std::vector<Walker> walkers;
    for (const ecs::Entity entity : units) {
        const auto* owner = world_.tryGet<components::Owner>(entity);
        auto* mover = world_.tryGet<components::Mover>(entity);
        auto* position = world_.tryGet<components::Position>(entity);
        if (owner == nullptr || owner->player != player || mover == nullptr || position == nullptr) {
            continue;
        }
        walkers.push_back(Walker{entity, mover, position, sizeOf(entity)});
    }

    for (const Walker& walker : walkers) {
        release(walker);
    }

    // Formation: big units pick first, then the closest ones; each takes the free square nearest the target.
    const components::Position goal = centerOf(target, 1);
    const auto distanceToGoal = [&goal](const Walker& walker) {
        const float dx = walker.position->x - goal.x;
        const float dy = walker.position->y - goal.y;
        return dx * dx + dy * dy;
    };
    std::sort(walkers.begin(), walkers.end(), [&](const Walker& a, const Walker& b) {
        return a.size != b.size ? a.size > b.size : distanceToGoal(a) < distanceToGoal(b);
    });

    std::vector<Walker> stranded;
    for (const Walker& walker : walkers) {
        const auto origins = pathfinding::nearestFreeTiles(centeredOrigin(target, walker.size), 1,
            [this, &walker](GridPos origin) { return !isSquareFreeDestination(origin, walker.size); });
        if (origins.empty() || !startPath(walker, origins.front())) {
            stranded.push_back(walker);
        }
    }
    // Settle last, so stranded units do not take squares meant for the others.
    for (const Walker& walker : stranded) {
        settle(walker);
    }
}

bool UnitSystem::update(float deltaSeconds)
{
    bool anyMoved = false;
    world_.each<components::Mover>([&](ecs::Entity entity, components::Mover& mover) {
        auto* position = world_.tryGet<components::Position>(entity);
        if (position == nullptr || !advance(Walker{entity, &mover, position, sizeOf(entity)}, deltaSeconds)) {
            return;
        }
        anyMoved = true;
        bus_.publish(events::UnitMoved{entity, position->x, position->y});
    });
    return anyMoved;
}

bool UnitSystem::isTileTaken(GridPos tile) const
{
    if (reserved_.contains(tile)) {
        return true;
    }

    bool covered = false;
    world_.each<components::Position>([&](ecs::Entity entity, const components::Position& position) {
        covered = covered || covers(position, sizeOf(entity), tile);
    });
    return covered;
}

std::optional<GridPos> UnitSystem::findSpawnSquare(
    GridPos center, int size, const std::vector<ecs::Entity>& ignored, int radius) const
{
    const auto isIgnored = [&ignored](ecs::Entity entity) {
        return std::find(ignored.begin(), ignored.end(), entity) != ignored.end();
    };
    const auto tileTaken = [&](GridPos tile) {
        if (buildings_.isOccupied(tile)) {
            return true;
        }
        if (const auto it = reserved_.find(tile); it != reserved_.end() && !isIgnored(it->second)) {
            return true;
        }
        bool covered = false;
        world_.each<components::Position>([&](ecs::Entity entity, const components::Position& position) {
            covered = covered || (!isIgnored(entity) && covers(position, sizeOf(entity), tile));
        });
        return covered;
    };

    const auto origins = pathfinding::nearestFreeTiles(centeredOrigin(center, size), 1,
        [&](GridPos origin) { return anyTileOf(origin, size, tileTaken); }, radius);
    if (origins.empty()) {
        return std::nullopt;
    }
    return origins.front();
}

int UnitSystem::sizeOf(ecs::Entity entity) const
{
    const auto* unit = world_.tryGet<components::Unit>(entity);
    return unit == nullptr ? 1 : unit->size;
}

bool UnitSystem::isSquareWalkable(GridPos origin, int size) const
{
    return !anyTileOf(origin, size, [this](GridPos tile) { return buildings_.isOccupied(tile); });
}

bool UnitSystem::isSquareFreeDestination(GridPos origin, int size) const
{
    return !anyTileOf(
        origin, size, [this](GridPos tile) { return buildings_.isOccupied(tile) || reserved_.contains(tile); });
}

bool UnitSystem::startPath(const Walker& walker, GridPos destination)
{
    const int size = walker.size;
    auto path = pathfinding::findPath(originOf(*walker.position, size), destination,
        [this, size](GridPos origin) { return !isSquareWalkable(origin, size); });
    if (!path) {
        return false;
    }

    // Already there: still walk to the exact center.
    if (path->empty()) {
        path->push_back(destination);
    }
    walker.mover->path = std::move(*path);
    walker.mover->next = 0;
    walker.mover->destination = destination;
    reserve(walker.entity, destination, size);
    return true;
}

void UnitSystem::settle(const Walker& walker)
{
    walker.mover->path.clear();
    walker.mover->next = 0;

    const GridPos here = originOf(*walker.position, walker.size);
    const auto origins = pathfinding::nearestFreeTiles(
        here, 1, [this, &walker](GridPos origin) { return !isSquareFreeDestination(origin, walker.size); });
    if (!origins.empty() && startPath(walker, origins.front())) {
        return;
    }
    walker.mover->destination = here;
    for (int dy = 0; dy < walker.size; ++dy) {
        for (int dx = 0; dx < walker.size; ++dx) {
            reserved_.try_emplace(GridPos{here.x + dx, here.y + dy}, walker.entity);
        }
    }
}

void UnitSystem::reserve(ecs::Entity entity, GridPos origin, int size)
{
    for (int dy = 0; dy < size; ++dy) {
        for (int dx = 0; dx < size; ++dx) {
            reserved_[GridPos{origin.x + dx, origin.y + dy}] = entity;
        }
    }
}

void UnitSystem::release(const Walker& walker)
{
    const GridPos origin = walker.mover->destination;
    for (int dy = 0; dy < walker.size; ++dy) {
        for (int dx = 0; dx < walker.size; ++dx) {
            const auto it = reserved_.find(GridPos{origin.x + dx, origin.y + dy});
            if (it != reserved_.end() && it->second == walker.entity) {
                reserved_.erase(it);
            }
        }
    }
}

bool UnitSystem::advance(const Walker& walker, float deltaSeconds)
{
    components::Mover& mover = *walker.mover;
    components::Position& position = *walker.position;
    if (mover.next >= mover.path.size()) {
        return false;
    }

    float budget = mover.speed * deltaSeconds;
    bool repathed = false;
    while (budget > 0.0f && mover.next < mover.path.size()) {
        const GridPos waypoint = mover.path[mover.next];
        if (!isSquareWalkable(waypoint, walker.size)) {
            // Something was built on the way: try a new route once, otherwise stop here.
            if (repathed || !startPath(walker, mover.destination)) {
                release(walker);
                settle(walker);
                return true;
            }
            repathed = true;
            continue;
        }

        const components::Position target = centerOf(waypoint, walker.size);
        const float dx = target.x - position.x;
        const float dy = target.y - position.y;
        const float distance = std::sqrt(dx * dx + dy * dy);
        if (distance <= budget) {
            position = target;
            budget -= distance;
            ++mover.next;
            continue;
        }
        position.x += dx / distance * budget;
        position.y += dy / distance * budget;
        budget = 0.0f;
    }

    if (mover.next >= mover.path.size()) {
        mover.path.clear();
        mover.next = 0;
    }
    return true;
}

} // namespace model
