#include "model/build/BuildingSystem.hpp"

#include "core/Events.hpp"
#include "model/build/BuildingComponents.hpp"
#include "model/economy/InventoryComponents.hpp"
#include "model/economy/ProductionComponents.hpp"
#include "model/economy/Item.hpp"
#include "model/units/UnitComponents.hpp"

#include <string>

namespace model {

BuildingSystem::BuildingSystem(core::EventBus& bus, ecs::Registry& world)
    : bus_(bus)
    , world_(world)
{
}

void BuildingSystem::reset()
{
    occupied_.clear();
    builders_.clear();
    commandCenters_.clear();
}

void BuildingSystem::initializePlayer(PlayerId player)
{
    publishBar(player);
    publishPreview(player);
}

void BuildingSystem::select(PlayerId player, BuildingType type)
{
    if (!isUnlocked(player, type)) {
        return;
    }

    auto& builder = builders_[player];
    if (builder.selected == type) {
        builder.selected.reset();
    } else {
        builder.selected = type;
    }
    publishBar(player);
    publishPreview(player);
}

void BuildingSystem::cancel(PlayerId player)
{
    auto& builder = builders_[player];
    if (!builder.selected) {
        return;
    }

    builder.selected.reset();
    publishBar(player);
    publishPreview(player);
}

void BuildingSystem::moveCursor(PlayerId player, std::optional<GridPos> tile)
{
    auto& builder = builders_[player];
    if (builder.cursor == tile) {
        return;
    }

    builder.cursor = tile;
    publishPreview(player);
}

void BuildingSystem::place(PlayerId player, GridPos anchor)
{
    auto& builder = builders_[player];
    if (!builder.selected) {
        return;
    }

    const BuildingType type = *builder.selected;
    const int size = buildingSpec(type).size;
    const GridPos origin = footprintOrigin(anchor, size);
    if (!canPlace(player, type, origin)) {
        return;
    }
    const ecs::Entity entity = world_.create();
    world_.emplace<components::Building>(entity, type);
    world_.emplace<components::Owner>(entity, player);
    world_.emplace<components::Inventory>(entity, std::size_t{64});
    world_.emplace<components::Footprint>(entity, origin, size);
    world_.emplace<components::Health>(entity, 500, 500);
    world_.emplace<components::Position>(entity, static_cast<float>(origin.x) + static_cast<float>(size) / 2.0F,
        static_cast<float>(origin.y) + static_cast<float>(size) / 2.0F);
    if (type == BuildingType::Tower) {
        world_.emplace<components::Combat>(entity, 20, 6.0F);
    }
    switch (type) {
    case BuildingType::IronMine:
        world_.emplace<components::Production>(entity, std::vector<ProductionInput>{}, ItemId::Iron, 1.0F);
        break;
    case BuildingType::CoalMine:
        world_.emplace<components::Production>(entity, std::vector<ProductionInput>{}, ItemId::Coal, 1.0F);
        break;
    case BuildingType::Steelworks:
        world_.emplace<components::Production>(entity,
            std::vector<ProductionInput>{{ItemId::Iron, 1.0F}, {ItemId::Coal, 1.0F}}, ItemId::Steel, 0.5F);
        break;
    case BuildingType::CopperMine:
        world_.emplace<components::Production>(entity, std::vector<ProductionInput>{}, ItemId::Copper, 1.0F);
        break;
    case BuildingType::ElectronicsFactory:
        world_.emplace<components::Production>(entity,
            std::vector<ProductionInput>{{ItemId::Iron, 1.0F}, {ItemId::Copper, 1.0F}}, ItemId::Electronics, 0.5F);
        break;
    case BuildingType::ToolFactory:
        world_.emplace<components::Production>(entity,
            std::vector<ProductionInput>{{ItemId::Steel, 1.0F}, {ItemId::Electronics, 1.0F}}, ItemId::Tools, 0.5F);
        break;
    case BuildingType::CommandCenter:
    case BuildingType::Wall:
    case BuildingType::House:
    case BuildingType::Generator:
    case BuildingType::Warehouse:
    case BuildingType::Barracks:
    case BuildingType::Cruzador:
    case BuildingType::Tower:
        break;
    }
    for (int dy = 0; dy < size; ++dy) {
        for (int dx = 0; dx < size; ++dx) {
            occupied_[GridPos{origin.x + dx, origin.y + dy}] = entity;
        }
    }
    if (type == BuildingType::CommandCenter) {
        commandCenters_[player] = entity;
    }
    bus_.publish(events::BuildingPlaced{entity, player, type, origin, size});

    // Keep the selection for repeated placement (walls) unless the type just got locked.
    if (!isUnlocked(player, type)) {
        builder.selected.reset();
    }
    publishBar(player);
    publishPreview(player);
}

void BuildingSystem::remove(ecs::Entity entity)
{
    const auto* footprint = world_.tryGet<components::Footprint>(entity);
    const auto* building = world_.tryGet<components::Building>(entity);
    const auto* owner = world_.tryGet<components::Owner>(entity);
    if (footprint == nullptr || building == nullptr || owner == nullptr) {
        return;
    }
    for (int dy = 0; dy < footprint->size; ++dy) {
        for (int dx = 0; dx < footprint->size; ++dx) {
            occupied_.erase(GridPos{footprint->origin.x + dx, footprint->origin.y + dy});
        }
    }
    if (building->type == BuildingType::CommandCenter) {
        commandCenters_.erase(owner->player);
    }
    world_.destroy(entity);
    bus_.publish(events::BuildingRemoved{entity});
    for (const auto& [player, builder] : builders_) {
        (void)builder;
        publishBar(player);
        publishPreview(player);
    }
}

void BuildingSystem::refreshPreviews() const
{
    for (const auto& [player, builder] : builders_) {
        if (builder.selected && builder.cursor) {
            publishPreview(player);
        }
    }
}

std::string_view BuildingSystem::lockedReason(PlayerId player, BuildingType type) const
{
    switch (type) {
    case BuildingType::CommandCenter:
        return hasCommandCenter(player) ? "Ja construido" : "";
    case BuildingType::Wall:
        return hasCommandCenter(player) ? "" : "Requer Centro";
    case BuildingType::House:
    case BuildingType::Generator:
    case BuildingType::Warehouse:
    case BuildingType::IronMine:
    case BuildingType::CoalMine:
    case BuildingType::Steelworks:
    case BuildingType::CopperMine:
    case BuildingType::ElectronicsFactory:
    case BuildingType::ToolFactory:
        return hasCommandCenter(player) ? "" : "Requer Centro";
    case BuildingType::Barracks:
    case BuildingType::Cruzador:
        return hasCommandCenter(player) ? "" : "Requer Centro";
    case BuildingType::Tower:
        return hasCommandCenter(player) ? "" : "Requer Centro";
    }
    return "";
}

bool BuildingSystem::isUnlocked(PlayerId player, BuildingType type) const
{
    return lockedReason(player, type).empty();
}

bool BuildingSystem::isAreaFree(GridPos origin, int size) const
{
    for (int dy = 0; dy < size; ++dy) {
        for (int dx = 0; dx < size; ++dx) {
            const GridPos tile{origin.x + dx, origin.y + dy};
            if (occupied_.contains(tile) || (tileBlocker_ && tileBlocker_(tile))) {
                return false;
            }
        }
    }
    return true;
}

bool BuildingSystem::canPlace(PlayerId player, BuildingType type, GridPos origin) const
{
    return isUnlocked(player, type) && isAreaFree(origin, buildingSpec(type).size);
}

bool BuildingSystem::hasCommandCenter(PlayerId player) const
{
    return commandCenters_.contains(player);
}

bool BuildingSystem::hasBuilding(PlayerId player, BuildingType type) const
{
    bool found = false;
    world_.each<components::Building>([&](ecs::Entity entity, const components::Building& building) {
        const auto* owner = world_.tryGet<components::Owner>(entity);
        found = found || (building.type == type && owner != nullptr && owner->player == player);
    });
    return found;
}

void BuildingSystem::publishBar(PlayerId player) const
{
    events::BuildBarChanged bar{player, {}, std::nullopt};
    if (const auto it = builders_.find(player); it != builders_.end()) {
        bar.selected = it->second.selected;
    }

    bar.entries.reserve(kBuildingTypes.size());
    for (const BuildingType type : kBuildingTypes) {
        const BuildingSpec spec = buildingSpec(type);
        const std::string_view reason = lockedReason(player, type);
        bar.entries.push_back(events::BuildBarEntry{
            type, std::string(spec.label), spec.size, reason.empty(), std::string(reason)});
    }
    bus_.publish(bar);
}

void BuildingSystem::publishPreview(PlayerId player) const
{
    events::BuildPreviewChanged preview{};
    preview.player = player;

    const auto it = builders_.find(player);
    if (it == builders_.end() || !it->second.selected || !it->second.cursor) {
        bus_.publish(preview);
        return;
    }

    const BuildingType type = *it->second.selected;
    preview.visible = true;
    preview.type = type;
    preview.size = buildingSpec(type).size;
    preview.origin = footprintOrigin(*it->second.cursor, preview.size);
    preview.valid = canPlace(player, type, preview.origin);
    bus_.publish(preview);
}

} // namespace model
