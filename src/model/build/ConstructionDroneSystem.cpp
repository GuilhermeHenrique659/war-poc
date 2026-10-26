#include "model/build/ConstructionDroneSystem.hpp"

#include "model/FootprintGeometry.hpp"
#include "model/build/BuildingComponents.hpp"
#include "model/build/ConstructionComponents.hpp"
#include "model/components/Owner.hpp"
#include "model/economy/InventorySystem.hpp"
#include "model/units/UnitComponents.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace model {

namespace {

[[nodiscard]] float distanceSquared(GridPos a, GridPos b)
{
    const float dx = static_cast<float>(a.x - b.x);
    const float dy = static_cast<float>(a.y - b.y);
    return dx * dx + dy * dy;
}

[[nodiscard]] float distanceSquared(const components::Position& position, GridPos target)
{
    const float dx = position.x - static_cast<float>(target.x);
    const float dy = position.y - static_cast<float>(target.y);
    return dx * dx + dy * dy;
}

[[nodiscard]] int costOf(const components::ConstructionSite& site, ItemId item)
{
    for (const BuildingCost& cost : buildingCosts(site.type)) {
        if (cost.item == item) {
            return std::max(0, cost.quantity);
        }
    }
    return 0;
}

} // namespace

ConstructionDroneSystem::ConstructionDroneSystem(
    core::EventBus& bus, ecs::Registry& world, UnitSystem& units)
    : bus_(bus)
    , world_(world)
    , units_(units)
{
    bus_.subscribe<events::BuildingPlaced>(
        [this](const events::BuildingPlaced& event) { onBuildingPlaced(event); });
    bus_.subscribe<events::DebugToggleRequested>(
        [this](const events::DebugToggleRequested&) { debugMode_ = !debugMode_; });
}

void ConstructionDroneSystem::reset()
{
    granted_.clear();
}

void ConstructionDroneSystem::onBuildingPlaced(const events::BuildingPlaced& event)
{
    if (event.type != BuildingType::CommandCenter) {
        return;
    }
    if (!granted_.insert(event.owner).second) {
        return;
    }
    const auto* footprint = world_.tryGet<components::Footprint>(event.entity);
    if (footprint == nullptr) {
        return;
    }
    (void)units_.spawnConstructionDrone(event.owner, footprintCenter(*footprint));
}

bool ConstructionDroneSystem::recruit(PlayerId player)
{
    const auto center = commandCenter(player);
    if (!center) {
        return false;
    }
    const auto* footprint = world_.tryGet<components::Footprint>(*center);
    if (footprint == nullptr) {
        return false;
    }

    components::Inventory* inventory = world_.tryGet<components::Inventory>(*center);
    int removedIron = 0;
    int removedElectronics = 0;
    if (!debugMode_) {
        if (inventory == nullptr
            || InventorySystem::amount(*inventory, ItemId::Iron) < kConstructionDroneIronCost
            || InventorySystem::amount(*inventory, ItemId::Electronics) < kConstructionDroneElectronicsCost) {
            return false;
        }
        removedIron = InventorySystem::remove(*inventory, ItemId::Iron, kConstructionDroneIronCost);
        removedElectronics =
            InventorySystem::remove(*inventory, ItemId::Electronics, kConstructionDroneElectronicsCost);
        if (removedIron != kConstructionDroneIronCost
            || removedElectronics != kConstructionDroneElectronicsCost) {
            (void)InventorySystem::add(*inventory, ItemId::Iron, removedIron);
            (void)InventorySystem::add(*inventory, ItemId::Electronics, removedElectronics);
            return false;
        }
    }

    const auto drone = units_.spawnConstructionDrone(player, footprintCenter(*footprint));
    if (!drone && !debugMode_) {
        (void)InventorySystem::add(*inventory, ItemId::Iron, removedIron);
        (void)InventorySystem::add(*inventory, ItemId::Electronics, removedElectronics);
    }
    return drone.has_value();
}

void ConstructionDroneSystem::update()
{
    // Debug construction ignores materials, so drones must not drain storages for it.
    if (debugMode_) {
        return;
    }

    std::vector<ecs::Entity> drones;
    world_.each<components::ConstructionDrone>(
        [&drones](ecs::Entity entity, const components::ConstructionDrone&) { drones.push_back(entity); });
    for (const ecs::Entity entity : drones) {
        if (auto* drone = world_.tryGet<components::ConstructionDrone>(entity)) {
            advanceDrone(entity, *drone);
        }
    }
}

void ConstructionDroneSystem::advanceDrone(ecs::Entity entity, components::ConstructionDrone& drone)
{
    const auto* owner = world_.tryGet<components::Owner>(entity);
    auto* cargo = world_.tryGet<components::Inventory>(entity);
    if (owner == nullptr || cargo == nullptr) {
        return;
    }

    switch (drone.state) {
    case components::ConstructionDroneState::Idle:
        if (drone.item && InventorySystem::amount(*cargo, *drone.item) > 0) {
            const auto site = nearestSiteNeeding(owner->player, *drone.item, entity);
            if (site) {
                drone.site = site;
                if (sendTo(entity, *site)) {
                    drone.state = components::ConstructionDroneState::ToSite;
                } else {
                    drone.site.reset();
                }
            }
            return;
        }
        chooseTask(entity, drone, owner->player);
        return;
    case components::ConstructionDroneState::ToSource:
        advanceToSource(entity, drone, *cargo);
        return;
    case components::ConstructionDroneState::ToSite:
        advanceToSite(entity, drone, *cargo);
        return;
    }
}

void ConstructionDroneSystem::chooseTask(
    ecs::Entity entity, components::ConstructionDrone& drone, PlayerId player)
{
    const auto* position = world_.tryGet<components::Position>(entity);
    if (position == nullptr) {
        return;
    }

    TaskCandidate best;
    bool found = false;
    world_.each<components::ConstructionSite>(
        [&](ecs::Entity siteEntity, const components::ConstructionSite& site) {
            const auto* owner = world_.tryGet<components::Owner>(siteEntity);
            const auto* footprint = world_.tryGet<components::Footprint>(siteEntity);
            if (owner == nullptr || owner->player != player || footprint == nullptr) {
                return;
            }
            const GridPos target = footprintCenter(*footprint);
            for (const BuildingCost& cost : buildingCosts(site.type)) {
                if (cost.quantity <= 0) {
                    continue;
                }
                const auto index = static_cast<std::size_t>(cost.item);
                const int remaining = cost.quantity - site.delivered[index] - site.pending[index];
                if (remaining <= 0) {
                    continue;
                }
                const auto source = nearestSource(player, cost.item, target, siteEntity);
                if (!source) {
                    continue;
                }
                const float ratio = static_cast<float>(site.delivered[index] + site.pending[index])
                    / static_cast<float>(cost.quantity);
                const bool better = !found || ratio < best.ratio
                    || (ratio == best.ratio
                        && (siteEntity < best.site
                            || (siteEntity == best.site && index < static_cast<std::size_t>(best.item))));
                if (!better) {
                    continue;
                }
                best = TaskCandidate{siteEntity, *source, cost.item, ratio};
                found = true;
            }
        });

    if (!found) {
        return;
    }
    drone.site = best.site;
    drone.source = best.source;
    drone.item = best.item;
    if (sendTo(entity, best.source)) {
        drone.state = components::ConstructionDroneState::ToSource;
        return;
    }
    drone.item.reset();
    drone.source.reset();
    drone.site.reset();
}

void ConstructionDroneSystem::advanceToSource(
    ecs::Entity entity, components::ConstructionDrone& drone, components::Inventory& cargo)
{
    if (!drone.site || !drone.source || !drone.item || !world_.isAlive(*drone.site)
        || !world_.has<components::ConstructionSite>(*drone.site) || !world_.isAlive(*drone.source)) {
        stopTask(drone, cargo);
        return;
    }

    const auto* mover = world_.tryGet<components::Mover>(entity);
    if (mover == nullptr || mover->next < mover->path.size()) {
        return;
    }

    auto* site = world_.tryGet<components::ConstructionSite>(*drone.site);
    auto* source = world_.tryGet<components::Inventory>(*drone.source);
    if (site == nullptr || source == nullptr) {
        stopTask(drone, cargo);
        return;
    }

    const auto index = static_cast<std::size_t>(*drone.item);
    const int remaining = costOf(*site, *drone.item) - site->delivered[index] - site->pending[index];
    const int amount = std::min({kConstructionDroneCapacity, remaining,
        InventorySystem::amount(*source, *drone.item)});
    if (amount <= 0) {
        stopTask(drone, cargo);
        return;
    }

    const int moved = InventorySystem::transfer(*source, cargo, *drone.item, amount);
    if (moved <= 0) {
        stopTask(drone, cargo);
        return;
    }
    site->pending[index] += moved;
    bus_.publish(events::DroneLoadStateChanged{entity, events::DroneLoadState::Loaded});
    drone.state = components::ConstructionDroneState::ToSite;
    if (!sendTo(entity, *drone.site)) {
        drone.site.reset();
        drone.source.reset();
        drone.state = components::ConstructionDroneState::Idle;
    }
}

void ConstructionDroneSystem::advanceToSite(
    ecs::Entity entity, components::ConstructionDrone& drone, components::Inventory& cargo)
{
    if (!drone.item || InventorySystem::amount(cargo, *drone.item) <= 0) {
        drone.item.reset();
        stopTask(drone, cargo);
        return;
    }
    if (!drone.site || !world_.isAlive(*drone.site)) {
        stopTask(drone, cargo);
        return;
    }

    const auto* mover = world_.tryGet<components::Mover>(entity);
    if (mover == nullptr || mover->next < mover->path.size()) {
        return;
    }

    auto* destination = world_.tryGet<components::Inventory>(*drone.site);
    if (destination == nullptr) {
        stopTask(drone, cargo);
        return;
    }

    const int carried = InventorySystem::amount(cargo, *drone.item);
    const int moved = InventorySystem::transfer(cargo, *destination, *drone.item, carried);
    if (moved <= 0) {
        return;
    }
    if (auto* site = world_.tryGet<components::ConstructionSite>(*drone.site)) {
        const auto index = static_cast<std::size_t>(*drone.item);
        site->pending[index] = std::max(0, site->pending[index] - moved);
        site->delivered[index] += moved;
    }
    if (InventorySystem::amount(cargo, *drone.item) > 0) {
        return;
    }
    drone.item.reset();
    bus_.publish(events::DroneLoadStateChanged{entity, events::DroneLoadState::Unloaded});
    stopTask(drone, cargo);
}

void ConstructionDroneSystem::stopTask(
    components::ConstructionDrone& drone, const components::Inventory& cargo) const
{
    drone.state = components::ConstructionDroneState::Idle;
    drone.site.reset();
    drone.source.reset();
    if (!drone.item || InventorySystem::amount(cargo, *drone.item) == 0) {
        drone.item.reset();
    }
}

std::optional<ecs::Entity> ConstructionDroneSystem::commandCenter(PlayerId player) const
{
    std::optional<ecs::Entity> found;
    world_.each<components::Building>([&](ecs::Entity entity, const components::Building& building) {
        const auto* owner = world_.tryGet<components::Owner>(entity);
        if (building.type == BuildingType::CommandCenter && owner != nullptr && owner->player == player
            && (!found || entity < *found)) {
            found = entity;
        }
    });
    return found;
}

std::optional<ecs::Entity> ConstructionDroneSystem::nearestSource(
    PlayerId player, ItemId item, GridPos target, ecs::Entity site) const
{
    std::optional<ecs::Entity> warehouse;
    std::optional<ecs::Entity> commandCenterEntity;
    float warehouseDistance = 0.0F;
    float commandCenterDistance = 0.0F;

    world_.each<components::Building>([&](ecs::Entity entity, const components::Building& building) {
        if (entity == site || (building.type != BuildingType::Warehouse
                                  && building.type != BuildingType::CommandCenter)) {
            return;
        }
        const auto* owner = world_.tryGet<components::Owner>(entity);
        const auto* footprint = world_.tryGet<components::Footprint>(entity);
        const auto* inventory = world_.tryGet<components::Inventory>(entity);
        if (owner == nullptr || owner->player != player || footprint == nullptr || inventory == nullptr
            || InventorySystem::amount(*inventory, item) <= 0) {
            return;
        }
        const float distance = distanceSquared(target, footprintCenter(*footprint));
        if (building.type == BuildingType::Warehouse) {
            if (!warehouse || distance < warehouseDistance
                || (distance == warehouseDistance && entity < *warehouse)) {
                warehouse = entity;
                warehouseDistance = distance;
            }
            return;
        }
        if (!commandCenterEntity || distance < commandCenterDistance
            || (distance == commandCenterDistance && entity < *commandCenterEntity)) {
            commandCenterEntity = entity;
            commandCenterDistance = distance;
        }
    });
    return warehouse ? warehouse : commandCenterEntity;
}

std::optional<ecs::Entity> ConstructionDroneSystem::nearestSiteNeeding(
    PlayerId player, ItemId item, ecs::Entity drone) const
{
    const auto* position = world_.tryGet<components::Position>(drone);
    if (position == nullptr) {
        return std::nullopt;
    }

    std::optional<ecs::Entity> found;
    float nearest = 0.0F;
    world_.each<components::ConstructionSite>(
        [&](ecs::Entity entity, const components::ConstructionSite& site) {
            const auto* owner = world_.tryGet<components::Owner>(entity);
            const auto* footprint = world_.tryGet<components::Footprint>(entity);
            if (owner == nullptr || owner->player != player || footprint == nullptr
                || costOf(site, item) <= 0) {
                return;
            }
            const auto index = static_cast<std::size_t>(item);
            if (site.delivered[index] >= costOf(site, item)) {
                return;
            }
            const float distance = distanceSquared(*position, footprintCenter(*footprint));
            if (!found || distance < nearest || (distance == nearest && entity < *found)) {
                found = entity;
                nearest = distance;
            }
        });
    return found;
}

bool ConstructionDroneSystem::sendTo(ecs::Entity drone, ecs::Entity building) const
{
    const auto* footprint = world_.tryGet<components::Footprint>(building);
    if (footprint == nullptr) {
        return false;
    }
    return units_.moveToAdjacent(drone, footprintCenter(*footprint));
}

} // namespace model
