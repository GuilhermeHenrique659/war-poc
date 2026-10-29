#include "model/build/BuildingSystem.hpp"

#include "core/Events.hpp"
#include "model/FootprintGeometry.hpp"
#include "model/StateNotifier.hpp"
#include "model/build/BuildingComponents.hpp"
#include "model/build/BuildingFactory.hpp"
#include "model/build/ConstructionComponents.hpp"
#include "model/combat/WeaponComponents.hpp"
#include "model/economy/InventoryComponents.hpp"
#include "model/economy/InventorySystem.hpp"
#include "model/economy/Market.hpp"
#include "model/economy/ProductionComponents.hpp"
#include "model/economy/Item.hpp"
#include "model/player/PlayerComponents.hpp"
#include "model/player/PlayerInventory.hpp"
#include "model/units/UnitComponents.hpp"

#include <algorithm>
#include <string>

namespace model {
namespace {

[[nodiscard]] std::vector<events::CostEntry> costEntries(BuildingType type)
{
    std::vector<events::CostEntry> entries;
    for (const BuildingCost& cost : buildingCosts(type)) {
        if (cost.item == ItemId::Count || cost.quantity <= 0) {
            continue;
        }
        entries.push_back(events::CostEntry{cost.item, cost.quantity});
    }
    return entries;
}

using PriceTable = std::array<double, static_cast<std::size_t>(ItemId::Count)>;

[[nodiscard]] double creditCostFor(BuildingType type, const PriceTable& prices)
{
    double total = 0.0;
    for (const BuildingCost& cost : buildingCosts(type)) {
        if (cost.item == ItemId::Count || cost.quantity <= 0) {
            continue;
        }
        total += prices[itemIndex(cost.item)] * static_cast<double>(cost.quantity);
    }
    return total;
}

} // namespace

BuildingSystem::BuildingSystem(core::EventBus& bus, ecs::Registry& world)
    : bus_(bus)
    , world_(world)
{
    for (const ItemId item : kAllItems) {
        prices_[itemIndex(item)] = marketDefinition(item).basePrice;
    }
    bus_.subscribe<events::DebugToggleRequested>([this](const events::DebugToggleRequested&) {
        debugMode_ = !debugMode_;
        for (const auto& [player, builder] : builders_) {
            (void)builder;
            publishBar(player);
        }
        refreshPreviews();
    });
    bus_.subscribe<events::MarketPricesChanged>([this](const events::MarketPricesChanged& event) {
        prices_ = event.prices;
        for (const auto& [player, builder] : builders_) {
            (void)builder;
            publishBar(player);
        }
    });
}

void BuildingSystem::reset()
{
    occupied_.clear();
    builders_.clear();
    commandCenters_.clear();
}

void BuildingSystem::initializePlayer(PlayerId player)
{
    builders_.try_emplace(player);
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
    world_.emplace<components::Owner>(entity, player);
    world_.emplace<components::Inventory>(entity, std::size_t{64});
    world_.emplace<components::Footprint>(entity, origin, size);
    const int health = buildingMaxHealth(type);
    world_.emplace<components::Health>(entity, health, health);
    const FootprintBounds bounds = footprintBounds(components::Footprint{origin, size});
    world_.emplace<components::Position>(entity, bounds.centerX(), bounds.centerY());
    for (int dy = 0; dy < size; ++dy) {
        for (int dx = 0; dx < size; ++dx) {
            occupied_[GridPos{origin.x + dx, origin.y + dy}] = entity;
        }
    }

    const float duration = buildingBuildSeconds(type);
    if (duration <= 0.0F) {
        completeBuilding(world_, entity, type);
        if (type == BuildingType::CommandCenter) {
            commandCenters_[player] = entity;
            transferStartingMaterials(player, entity);
        }
        bus_.publish(events::BuildingPlaced{entity, player, type, origin, size});
        setBuildingState(bus_, world_, entity, BuildingState::Idle);
    } else {
        world_.emplace<components::ConstructionSite>(entity, type, duration);
        bus_.publish(events::ConstructionStarted{entity, player, type, origin, size, duration});
        setBuildingState(bus_, world_, entity, BuildingState::UnderConstruction);
    }

    // Keep the selection for repeated placement (walls) unless the type just got locked.
    if (!isUnlocked(player, type) || type == BuildingType::TrainingDummy) {
        builder.selected.reset();
    }
    publishBar(player);
    publishPreview(player);
}

void BuildingSystem::remove(ecs::Entity entity)
{
    const auto* footprint = world_.tryGet<components::Footprint>(entity);
    const auto* building = world_.tryGet<components::Building>(entity);
    const auto* site = world_.tryGet<components::ConstructionSite>(entity);
    const auto* owner = world_.tryGet<components::Owner>(entity);
    if (footprint == nullptr || owner == nullptr || (building == nullptr && site == nullptr)) {
        return;
    }
    const BuildingType type = building != nullptr ? building->type : site->type;
    for (int dy = 0; dy < footprint->size; ++dy) {
        for (int dx = 0; dx < footprint->size; ++dx) {
            occupied_.erase(GridPos{footprint->origin.x + dx, footprint->origin.y + dy});
        }
    }
    if (type == BuildingType::CommandCenter) {
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

events::BuildLockReason BuildingSystem::lockReason(PlayerId player, BuildingType type) const
{
    if (type == BuildingType::CommandCenter) {
        return hasCommandCenter(player) ? events::BuildLockReason::AlreadyBuilt
                                        : events::BuildLockReason::None;
    }
    return hasCommandCenter(player) ? events::BuildLockReason::None
                                    : events::BuildLockReason::RequiresCommandCenter;
}

bool BuildingSystem::isUnlocked(PlayerId player, BuildingType type) const
{
    return lockReason(player, type) == events::BuildLockReason::None;
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
    const int size = buildingSpec(type).size;
    return isUnlocked(player, type) && isAreaFree(origin, size)
        && (!placementRule_ || placementRule_(type, origin, size));
}

bool BuildingSystem::hasCommandCenter(PlayerId player) const
{
    return commandCenters_.contains(player);
}

components::Inventory* BuildingSystem::playerInventory(PlayerId player) const
{
    return model::playerInventory(world_, player);
}

void BuildingSystem::transferStartingMaterials(PlayerId player, ecs::Entity commandCenter) const
{
    components::Inventory* source = playerInventory(player);
    components::Inventory* destination = world_.tryGet<components::Inventory>(commandCenter);
    if (source == nullptr || destination == nullptr) {
        return;
    }
    for (const ItemId item : kConstructionMaterials) {
        (void)InventorySystem::transfer(*source, *destination, item, InventorySystem::amount(*source, item));
    }
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
        const events::BuildLockReason reason = lockReason(player, type);
        bar.entries.push_back(events::BuildBarEntry{
            type, spec.size, reason == events::BuildLockReason::None, reason, costEntries(type), debugMode_,
            creditCostFor(type, prices_)});
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
