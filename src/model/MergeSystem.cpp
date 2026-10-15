#include "model/MergeSystem.hpp"

#include "model/components/UnitComponents.hpp"

#include <algorithm>
#include <cmath>

namespace model {

namespace {

// Every unit must be this close (in tiles) to the group's center, and the Combined unit
// spawns at most this many rings away from it.
constexpr float kMergeRadius = 3.0f;

} // namespace

MergeSystem::MergeSystem(core::EventBus& bus, ecs::Registry& world, UnitSystem& units, SelectionSystem& selection)
    : bus_(bus)
    , world_(world)
    , units_(units)
    , selection_(selection)
{
}

void MergeSystem::reset()
{
    published_.clear();
}

void MergeSystem::initializePlayer(PlayerId player)
{
    events::MergePanelChanged closed{};
    closed.player = player;
    published_[player] = closed;
    bus_.publish(closed);
}

void MergeSystem::refresh(PlayerId player)
{
    const Plan current = plan(player);

    events::MergePanelChanged panel{};
    panel.player = player;
    panel.open = current.units.size() >= 2;
    if (panel.open) {
        panel.unitCount = static_cast<int>(current.units.size());
        panel.canMerge = current.blockedReason.empty();
        panel.blockedReason = current.blockedReason;
        panel.maxHealth = current.result.maxHealth;
        panel.damage = current.result.damage;
        panel.range = current.result.range;
        panel.speed = current.result.speed;
        panel.size = current.result.size;
    }

    const auto it = published_.find(player);
    if (it != published_.end() && it->second == panel) {
        return;
    }
    published_[player] = panel;
    bus_.publish(panel);
}

void MergeSystem::refreshAll()
{
    std::vector<PlayerId> players;
    players.reserve(published_.size());
    for (const auto& [player, panel] : published_) {
        players.push_back(player);
    }
    for (const PlayerId player : players) {
        refresh(player);
    }
}

void MergeSystem::merge(PlayerId player)
{
    const Plan current = plan(player);
    if (current.units.size() < 2 || !current.blockedReason.empty() || !current.origin) {
        return;
    }

    for (const ecs::Entity entity : current.units) {
        units_.removeForMerge(entity);
    }
    const ecs::Entity merged = units_.spawn(player, current.result, *current.origin);
    selection_.selectUnits(player, {merged}, false);
    refresh(player);
}

MergeSystem::Plan MergeSystem::plan(PlayerId player) const
{
    Plan result;
    for (const ecs::Entity entity : selection_.selectedUnits(player)) {
        const auto* owner = world_.tryGet<components::Owner>(entity);
        if (owner == nullptr || owner->player != player || !world_.has<components::Unit>(entity)
            || !world_.has<components::Health>(entity) || !world_.has<components::Combat>(entity)
            || !world_.has<components::Mover>(entity) || !world_.has<components::Position>(entity)) {
            continue;
        }
        result.units.push_back(entity);
    }
    if (result.units.size() < 2) {
        return result;
    }

    // Health and damage add up, range is the best one and speed is the average of the
    // recruited units inside (so merging in steps or all at once gives the same unit).
    UnitStats& stats = result.result;
    stats = UnitStats{UnitType::Combined, 0, 0, 0, 0.0f, 0.0f, 0, 1};
    float weightedSpeed = 0.0f;
    float centerX = 0.0f;
    float centerY = 0.0f;
    for (const ecs::Entity entity : result.units) {
        const auto* unit = world_.tryGet<components::Unit>(entity);
        const auto* health = world_.tryGet<components::Health>(entity);
        const auto* combat = world_.tryGet<components::Combat>(entity);
        const auto* mover = world_.tryGet<components::Mover>(entity);
        const auto* position = world_.tryGet<components::Position>(entity);

        stats.health += health->current;
        stats.maxHealth += health->max;
        stats.damage += combat->damage;
        stats.range = std::max(stats.range, combat->range);
        stats.baseCount += unit->baseCount;
        weightedSpeed += mover->speed * static_cast<float>(unit->baseCount);
        centerX += position->x;
        centerY += position->y;
    }
    stats.speed = weightedSpeed / static_cast<float>(stats.baseCount);
    stats.size = unitSizeForPower(unitPower(stats.maxHealth, stats.damage));

    const float count = static_cast<float>(result.units.size());
    centerX /= count;
    centerY /= count;
    for (const ecs::Entity entity : result.units) {
        const auto* position = world_.tryGet<components::Position>(entity);
        if (std::hypot(position->x - centerX, position->y - centerY) > kMergeRadius) {
            result.blockedReason = "Muito longe";
            return result;
        }
    }

    const GridPos centerTile{static_cast<int>(std::floor(centerX)), static_cast<int>(std::floor(centerY))};
    result.origin = units_.findSpawnSquare(centerTile, stats.size, result.units, static_cast<int>(kMergeRadius));
    if (!result.origin) {
        result.blockedReason = "Sem espaco";
    }
    return result;
}

} // namespace model
