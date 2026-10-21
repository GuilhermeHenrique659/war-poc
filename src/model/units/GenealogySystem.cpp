#include "model/units/GenealogySystem.hpp"

#include "model/build/BuildingComponents.hpp"
#include "model/units/UnitComponents.hpp"

#include <algorithm>
#include <cmath>

namespace model {

namespace {

constexpr float kArrivalRadius = 2.5F;
constexpr int kSpawnRadius = 3;

[[nodiscard]] UnitStats legacyChildStats(
    const std::vector<ecs::Entity>& parents, ecs::Registry& world)
{
    UnitStats stats{UnitType::Combined, 0, 0, 0, 0.0F, 0.0F, 0, 1};
    float weightedSpeed = 0.0F;
    for (const ecs::Entity entity : parents) {
        const auto* unit = world.tryGet<components::Unit>(entity);
        const auto* health = world.tryGet<components::Health>(entity);
        const auto* combat = world.tryGet<components::Combat>(entity);
        const auto* mover = world.tryGet<components::Mover>(entity);
        if (unit == nullptr || health == nullptr || combat == nullptr || mover == nullptr) {
            return UnitStats{UnitType::Combined, 0, 0, 0, 0.0F, 0.0F, 0, 1};
        }
        stats.health += health->current;
        stats.maxHealth += health->max;
        stats.damage += combat->damage;
        stats.range = std::max(stats.range, combat->range);
        stats.baseCount += unit->baseCount;
        weightedSpeed += mover->speed * static_cast<float>(unit->baseCount);
    }
    if (stats.baseCount <= 0) {
        return stats;
    }
    stats.speed = weightedSpeed / static_cast<float>(stats.baseCount);
    stats.size = unitSizeForPower(unitPower(stats.maxHealth, stats.damage));
    return stats;
}

} // namespace

GenealogySystem::GenealogySystem(ecs::Registry& world, UnitSystem& units, SelectionSystem& selection)
    : world_(world)
    , units_(units)
    , selection_(selection)
{
}

void GenealogySystem::reset()
{
    pending_.clear();
}

bool GenealogySystem::send(PlayerId player, ecs::Entity cruzador, const std::vector<ecs::Entity>& units)
{
    if (units.size() != 2 || units[0] == units[1]) {
        return false;
    }
    const auto* building = world_.tryGet<components::Building>(cruzador);
    const auto* buildingOwner = world_.tryGet<components::Owner>(cruzador);
    const auto* footprint = world_.tryGet<components::Footprint>(cruzador);
    if (building == nullptr || building->type != BuildingType::Cruzador || buildingOwner == nullptr
        || buildingOwner->player != player || footprint == nullptr) {
        return false;
    }
    for (const ecs::Entity entity : units) {
        const auto* owner = world_.tryGet<components::Owner>(entity);
        if (owner == nullptr || owner->player != player || !world_.has<components::Unit>(entity)
            || !world_.has<components::Health>(entity) || !world_.has<components::Combat>(entity)
            || !world_.has<components::Mover>(entity) || !world_.has<components::Position>(entity)) {
            return false;
        }
        if (std::any_of(pending_.begin(), pending_.end(), [entity](const PendingPair& pair) {
                return std::find(pair.units.begin(), pair.units.end(), entity) != pair.units.end();
            })) {
            return false;
        }
    }

    const GridPos entrance{footprint->origin.x - 1, footprint->origin.y + footprint->size / 2};
    units_.orderMove(player, units, entrance);
    const auto* firstMover = world_.tryGet<components::Mover>(units[0]);
    const auto* secondMover = world_.tryGet<components::Mover>(units[1]);
    if (firstMover == nullptr || secondMover == nullptr) {
        return false;
    }
    pending_.push_back(PendingPair{player, cruzador, units, {firstMover->destination, secondMover->destination}});
    return true;
}

void GenealogySystem::update()
{
    std::erase_if(pending_, [this](const PendingPair& pair) {
        if (!world_.isAlive(pair.cruzador) || !world_.isAlive(pair.units[0]) || !world_.isAlive(pair.units[1])) {
            return true;
        }
        for (std::size_t index = 0; index < pair.units.size(); ++index) {
            const auto* mover = world_.tryGet<components::Mover>(pair.units[index]);
            if (mover == nullptr || mover->destination != pair.destinations[index]) {
                return true;
            }
        }
        if (!arrived(pair)) {
            return !std::any_of(pair.units.begin(), pair.units.end(), [this](ecs::Entity entity) {
                const auto* mover = world_.tryGet<components::Mover>(entity);
                return mover == nullptr || !mover->path.empty();
            });
        }
        return createChild(pair);
    });
}

bool GenealogySystem::arrived(const PendingPair& pair) const
{
    const auto* footprint = world_.tryGet<components::Footprint>(pair.cruzador);
    if (footprint == nullptr) {
        return false;
    }
    const float left = static_cast<float>(footprint->origin.x);
    const float top = static_cast<float>(footprint->origin.y);
    const float right = left + static_cast<float>(footprint->size);
    const float bottom = top + static_cast<float>(footprint->size);
    for (std::size_t index = 0; index < pair.units.size(); ++index) {
        const ecs::Entity entity = pair.units[index];
        const auto* mover = world_.tryGet<components::Mover>(entity);
        const auto* position = world_.tryGet<components::Position>(entity);
        if (mover == nullptr || position == nullptr || mover->destination != pair.destinations[index]
            || !mover->path.empty()) {
            return false;
        }
        const float nearestX = std::clamp(position->x, left, right);
        const float nearestY = std::clamp(position->y, top, bottom);
        if (std::hypot(position->x - nearestX, position->y - nearestY) > kArrivalRadius) {
            return false;
        }
    }
    return true;
}

bool GenealogySystem::createChild(const PendingPair& pair)
{
    const auto* footprint = world_.tryGet<components::Footprint>(pair.cruzador);
    if (footprint == nullptr) {
        return false;
    }
    const UnitStats stats = legacyChildStats(pair.units, world_);
    if (stats.baseCount <= 0) {
        return false;
    }
    const GridPos center{footprint->origin.x + footprint->size / 2, footprint->origin.y + footprint->size / 2};
    const auto origin = units_.findSpawnSquare(center, stats.size, pair.units, kSpawnRadius);
    if (!origin) {
        return false;
    }

    for (const ecs::Entity entity : pair.units) {
        units_.removeForCombination(entity);
    }
    const ecs::Entity child = units_.spawn(pair.player, stats, *origin);
    selection_.selectUnits(pair.player, {child}, false);
    return true;
}

} // namespace model
