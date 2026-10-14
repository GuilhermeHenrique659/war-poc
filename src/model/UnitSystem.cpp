#include "model/UnitSystem.hpp"

#include "core/Events.hpp"
#include "model/Pathfinding.hpp"
#include "model/components/BuildingComponents.hpp"

#include <cmath>
#include <cstddef>
#include <limits>
#include <optional>
#include <utility>

namespace model {

namespace {

struct Walker {
    ecs::Entity entity;
    components::Mover* mover;
    components::Position* position;
};

[[nodiscard]] GridPos tileOf(const components::Position& position)
{
    return GridPos{static_cast<int>(std::floor(position.x)), static_cast<int>(std::floor(position.y))};
}

[[nodiscard]] float distanceSquared(const components::Position& position, GridPos tile)
{
    const float dx = static_cast<float>(tile.x) + 0.5f - position.x;
    const float dy = static_cast<float>(tile.y) + 0.5f - position.y;
    return dx * dx + dy * dy;
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

    const GridPos center{footprint->origin.x + footprint->size / 2, footprint->origin.y + footprint->size / 2};
    const auto tiles = pathfinding::nearestFreeTiles(
        center, 1, [this](GridPos tile) { return !isWalkable(tile) || isTileTaken(tile); });
    if (tiles.empty()) {
        return;
    }

    const GridPos tile = tiles.front();
    const UnitSpec spec = unitSpec(type);
    const components::Position position{static_cast<float>(tile.x) + 0.5f, static_cast<float>(tile.y) + 0.5f};
    components::Mover mover{spec.speed, {}, 0, tile};

    const ecs::Entity entity = world_.create();
    world_.emplace<components::Unit>(entity, type);
    world_.emplace<components::Owner>(entity, player);
    world_.emplace<components::Health>(entity, spec.maxHealth, spec.maxHealth);
    world_.emplace<components::Combat>(entity, spec.damage, spec.range);
    world_.emplace<components::Position>(entity, position);
    world_.emplace<components::Mover>(entity, std::move(mover));
    reserved_[tile] = entity;

    bus_.publish(events::UnitSpawned{
        entity, player, type, position.x, position.y, spec.maxHealth, spec.maxHealth});
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
        walkers.push_back(Walker{entity, mover, position});
    }
    if (walkers.empty()) {
        return;
    }

    for (const Walker& walker : walkers) {
        release(walker.entity, *walker.mover);
    }

    // Formation: the nearest free tiles around the target, each taken by the closest unassigned unit.
    const auto tiles = pathfinding::nearestFreeTiles(
        target, walkers.size(), [this](GridPos tile) { return !isFreeDestination(tile); });
    std::vector<bool> assigned(walkers.size(), false);
    std::vector<std::size_t> stranded;
    for (const GridPos tile : tiles) {
        std::size_t best = walkers.size();
        float bestDistance = std::numeric_limits<float>::max();
        for (std::size_t i = 0; i < walkers.size(); ++i) {
            const float distance = distanceSquared(*walkers[i].position, tile);
            if (!assigned[i] && distance < bestDistance) {
                best = i;
                bestDistance = distance;
            }
        }
        if (best == walkers.size()) {
            break;
        }
        assigned[best] = true;
        const Walker& walker = walkers[best];
        if (!startPath(walker.entity, *walker.mover, *walker.position, tile)) {
            stranded.push_back(best);
        }
    }

    for (std::size_t i = 0; i < walkers.size(); ++i) {
        if (!assigned[i]) {
            stranded.push_back(i);
        }
    }
    // Settle last, so stranded units do not take tiles meant for the others.
    for (const std::size_t i : stranded) {
        settle(walkers[i].entity, *walkers[i].mover, *walkers[i].position);
    }
}

bool UnitSystem::update(float deltaSeconds)
{
    bool anyMoved = false;
    world_.each<components::Mover>([&](ecs::Entity entity, components::Mover& mover) {
        auto* position = world_.tryGet<components::Position>(entity);
        if (position == nullptr || !advance(entity, mover, *position, deltaSeconds)) {
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

    bool standing = false;
    world_.each<components::Position>([&](ecs::Entity, const components::Position& position) {
        standing = standing || tileOf(position) == tile;
    });
    return standing;
}

bool UnitSystem::isWalkable(GridPos tile) const
{
    return !buildings_.isOccupied(tile);
}

bool UnitSystem::isFreeDestination(GridPos tile) const
{
    return isWalkable(tile) && !reserved_.contains(tile);
}

bool UnitSystem::startPath(
    ecs::Entity entity, components::Mover& mover, const components::Position& position, GridPos destination)
{
    auto path = pathfinding::findPath(
        tileOf(position), destination, [this](GridPos tile) { return !isWalkable(tile); });
    if (!path) {
        return false;
    }

    // Already on the tile: still walk to its center.
    if (path->empty()) {
        path->push_back(destination);
    }
    mover.path = std::move(*path);
    mover.next = 0;
    mover.destination = destination;
    reserved_[destination] = entity;
    return true;
}

void UnitSystem::settle(ecs::Entity entity, components::Mover& mover, const components::Position& position)
{
    mover.path.clear();
    mover.next = 0;

    const GridPos here = tileOf(position);
    const auto tiles = pathfinding::nearestFreeTiles(
        here, 1, [this](GridPos tile) { return !isFreeDestination(tile); });
    if (!tiles.empty() && startPath(entity, mover, position, tiles.front())) {
        return;
    }
    mover.destination = here;
    reserved_.try_emplace(here, entity);
}

void UnitSystem::release(ecs::Entity entity, const components::Mover& mover)
{
    const auto it = reserved_.find(mover.destination);
    if (it != reserved_.end() && it->second == entity) {
        reserved_.erase(it);
    }
}

bool UnitSystem::advance(
    ecs::Entity entity, components::Mover& mover, components::Position& position, float deltaSeconds)
{
    if (mover.next >= mover.path.size()) {
        return false;
    }

    float budget = mover.speed * deltaSeconds;
    bool repathed = false;
    while (budget > 0.0f && mover.next < mover.path.size()) {
        const GridPos waypoint = mover.path[mover.next];
        if (!isWalkable(waypoint)) {
            // Something was built on the way: try a new route once, otherwise stop here.
            if (repathed || !startPath(entity, mover, position, mover.destination)) {
                release(entity, mover);
                settle(entity, mover, position);
                return true;
            }
            repathed = true;
            continue;
        }

        const float dx = static_cast<float>(waypoint.x) + 0.5f - position.x;
        const float dy = static_cast<float>(waypoint.y) + 0.5f - position.y;
        const float distance = std::sqrt(dx * dx + dy * dy);
        if (distance <= budget) {
            position.x += dx;
            position.y += dy;
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
