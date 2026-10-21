#include "model/selection/SelectionSystem.hpp"

#include "core/Events.hpp"
#include "model/units/UnitType.hpp"
#include "model/build/BuildingComponents.hpp"
#include "model/units/UnitComponents.hpp"

#include <algorithm>
#include <string>

namespace model {

SelectionSystem::SelectionSystem(core::EventBus& bus, ecs::Registry& world)
    : bus_(bus)
    , world_(world)
{
    bus_.subscribe<events::UnitRemoved>([this](const auto& event) { removeEntity(event.entity); });
    bus_.subscribe<events::BuildingRemoved>([this](const auto& event) { removeEntity(event.entity); });
}

void SelectionSystem::reset()
{
    selections_.clear();
}

void SelectionSystem::initializePlayer(PlayerId player)
{
    publish(player);
}

void SelectionSystem::selectUnits(PlayerId player, const std::vector<ecs::Entity>& units, bool additive)
{
    auto& selection = selections_[player];
    if (!additive) {
        selection.units.clear();
    }
    selection.building.reset();

    for (const ecs::Entity entity : units) {
        if (!world_.has<components::Unit>(entity) || !isOwnedBy(entity, player)) {
            continue;
        }
        if (std::find(selection.units.begin(), selection.units.end(), entity) != selection.units.end()) {
            continue;
        }
        selection.units.push_back(entity);
    }
    publish(player);
}

void SelectionSystem::selectBuilding(PlayerId player, ecs::Entity building)
{
    if (!world_.has<components::Building>(building) || !isOwnedBy(building, player)) {
        return;
    }

    auto& selection = selections_[player];
    selection.units.clear();
    selection.building = building;
    publish(player);
}

void SelectionSystem::clear(PlayerId player)
{
    auto& selection = selections_[player];
    if (selection.units.empty() && !selection.building) {
        return;
    }

    selection = PlayerSelection{};
    publish(player);
}

std::vector<ecs::Entity> SelectionSystem::selectedUnits(PlayerId player) const
{
    const auto it = selections_.find(player);
    return it == selections_.end() ? std::vector<ecs::Entity>{} : it->second.units;
}

std::optional<ecs::Entity> SelectionSystem::selectedCommandCenter(PlayerId player) const
{
    const auto it = selections_.find(player);
    if (it == selections_.end() || !it->second.building) {
        return std::nullopt;
    }

    const ecs::Entity entity = *it->second.building;
    const auto* building = world_.tryGet<components::Building>(entity);
    if (building == nullptr || building->type != BuildingType::CommandCenter) {
        return std::nullopt;
    }
    return entity;
}

bool SelectionSystem::isOwnedBy(ecs::Entity entity, PlayerId player) const
{
    const auto* owner = world_.tryGet<components::Owner>(entity);
    return owner != nullptr && owner->player == player;
}

void SelectionSystem::removeEntity(ecs::Entity entity)
{
    for (auto& [player, selection] : selections_) {
        const auto oldSize = selection.units.size();
        std::erase(selection.units, entity);
        const bool removedUnit = selection.units.size() != oldSize;
        const bool removedBuilding = selection.building == entity;
        if (!removedUnit && !removedBuilding) {
            continue;
        }
        if (removedBuilding) {
            selection.building.reset();
        }
        publish(player);
    }
}

void SelectionSystem::publish(PlayerId player) const
{
    events::SelectionChanged selection{player, {}, std::nullopt};
    if (const auto it = selections_.find(player); it != selections_.end()) {
        selection.units = it->second.units;
        selection.building = it->second.building;
    }
    bus_.publish(selection);

    events::RecruitPanelChanged panel{player, selectedCommandCenter(player).has_value(), {}};
    if (panel.open) {
        for (const UnitType type : kRecruitableUnitTypes) {
            const UnitSpec spec = unitSpec(type);
            panel.entries.push_back(events::RecruitEntry{
                type, std::string(spec.label), spec.maxHealth, spec.damage, spec.range, spec.speed});
        }
    }
    bus_.publish(panel);
}

} // namespace model
