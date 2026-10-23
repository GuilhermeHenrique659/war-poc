#include "model/economy/LogisticsSystem.hpp"

#include "core/Events.hpp"
#include "model/build/BuildingComponents.hpp"
#include "model/components/Owner.hpp"
#include "model/economy/InventoryComponents.hpp"
#include "model/economy/InventorySystem.hpp"
#include "model/economy/ProductionComponents.hpp"
#include "model/player/PlayerComponents.hpp"
#include "model/units/UnitComponents.hpp"

#include <algorithm>
#include <utility>
#include <vector>

namespace model {

LogisticsSystem::LogisticsSystem(core::EventBus& bus, ecs::Registry& world, UnitSystem& units)
    : bus_(bus)
    , world_(world)
    , units_(units)
{
    bus_.subscribe<events::BuildingPlaced>([this](const auto& event) { onBuildingPlaced(event); });
    bus_.subscribe<events::BuildingRemoved>([this](const auto& event) { onBuildingRemoved(event); });
    bus_.subscribe<events::DebugToggleRequested>([this](const events::DebugToggleRequested&) {
        debugMode_ = !debugMode_;
    });
}

void LogisticsSystem::reset()
{
    nextRouteId_ = 1;
}

void LogisticsSystem::onBuildingPlaced(const events::BuildingPlaced& event)
{
    const auto* owner = world_.tryGet<components::Owner>(event.entity);
    const auto* building = world_.tryGet<components::Building>(event.entity);
    const auto* production = world_.tryGet<components::Production>(event.entity);
    if (owner == nullptr || building == nullptr) {
        return;
    }

    const auto ownedProduction = [this, player = owner->player](ecs::Entity entity) {
        const auto* entityOwner = world_.tryGet<components::Owner>(entity);
        return entityOwner != nullptr && entityOwner->player == player
            && world_.tryGet<components::Production>(entity) != nullptr;
    };
    const auto matchesInput = [this](ecs::Entity destination, ItemId item) {
        const auto* consumer = world_.tryGet<components::Production>(destination);
        return consumer != nullptr
            && std::any_of(consumer->inputs.begin(), consumer->inputs.end(),
                [item](const ProductionInput& input) { return input.item == item; });
    };

    if (production != nullptr) {
        world_.each<components::Building>([&](ecs::Entity destination, const components::Building& target) {
            if (destination == event.entity || !ownedBy(destination, owner->player)) {
                return;
            }
            if (target.type == BuildingType::Warehouse || matchesInput(destination, production->output)) {
                createRoute(event.entity, destination, production->output);
            }
        });
    }

    if (building->type == BuildingType::Warehouse || (production != nullptr && !production->inputs.empty())) {
        world_.each<components::Production>([&](ecs::Entity source, const components::Production& producer) {
            if (source == event.entity || !ownedProduction(source)) {
                return;
            }
            const bool warehouse = building->type == BuildingType::Warehouse;
            const bool matchingInput = matchesInput(event.entity, producer.output);
            if (warehouse || matchingInput) {
                createRoute(source, event.entity, producer.output);
            }
        });
    }
}

void LogisticsSystem::onBuildingRemoved(const events::BuildingRemoved& event)
{
    std::vector<ecs::Entity> dronesToDetach;
    world_.each<components::TransportDrone>([&](ecs::Entity entity, const components::TransportDrone& drone) {
        if (!drone.routeId) {
            return;
        }
        const components::LogisticsRoute* route = nullptr;
        const auto source = buildingForRoute(*drone.routeId);
        if (source) {
            const auto* routes = world_.tryGet<components::LogisticsRoutes>(*source);
            if (routes != nullptr) {
                const auto it = std::find_if(routes->routes.begin(), routes->routes.end(),
                    [&drone](const components::LogisticsRoute& entry) { return entry.id == *drone.routeId; });
                if (it != routes->routes.end()) {
                    route = &*it;
                }
            }
        }
        if (entity == event.entity || route == nullptr || route->source == event.entity
            || route->destination == event.entity) {
            dronesToDetach.push_back(entity);
        }
    });
    for (const ecs::Entity entity : dronesToDetach) {
        if (auto* drone = world_.tryGet<components::TransportDrone>(entity)) {
            detachDrone(entity, *drone);
        }
    }

    world_.each<components::LogisticsRoutes>([&](ecs::Entity source, components::LogisticsRoutes& routes) {
        if (source == event.entity) {
            return;
        }
        std::erase_if(routes.routes, [&event](const components::LogisticsRoute& route) {
            return route.destination == event.entity;
        });
    });
}

void LogisticsSystem::createRoute(ecs::Entity source, ecs::Entity destination, ItemId item)
{
    if (source == destination || item == ItemId::Count) {
        return;
    }
    auto* routes = world_.tryGet<components::LogisticsRoutes>(source);
    if (routes == nullptr) {
        routes = &world_.emplace<components::LogisticsRoutes>(source);
    }
    const auto existing = std::find_if(routes->routes.begin(), routes->routes.end(),
        [destination, item](const components::LogisticsRoute& route) {
            return route.destination == destination && route.item == item;
        });
    if (existing != routes->routes.end()) {
        return;
    }
    routes->routes.push_back(components::LogisticsRoute{nextRouteId_++, source, destination, item});
}

components::LogisticsRoute* LogisticsSystem::findRoute(
    ecs::Entity source, components::LogisticsRouteId routeId) const
{
    auto* routes = world_.tryGet<components::LogisticsRoutes>(source);
    if (routes == nullptr) {
        return nullptr;
    }
    const auto route = std::find_if(routes->routes.begin(), routes->routes.end(),
        [routeId](const components::LogisticsRoute& entry) { return entry.id == routeId; });
    return route == routes->routes.end() ? nullptr : &*route;
}

std::optional<ecs::Entity> LogisticsSystem::buildingForRoute(components::LogisticsRouteId routeId) const
{
    std::optional<ecs::Entity> source;
    world_.each<components::LogisticsRoutes>([&](ecs::Entity entity, const components::LogisticsRoutes& routes) {
        const bool containsRoute = std::any_of(routes.routes.begin(), routes.routes.end(),
            [routeId](const components::LogisticsRoute& route) { return route.id == routeId; });
        if (containsRoute) {
            source = entity;
        }
    });
    return source;
}

bool LogisticsSystem::ownedBy(ecs::Entity entity, PlayerId player) const
{
    const auto* owner = world_.tryGet<components::Owner>(entity);
    return owner != nullptr && owner->player == player;
}

components::Inventory* LogisticsSystem::playerInventory(PlayerId player) const
{
    components::Inventory* inventory = nullptr;
    world_.each<components::PlayerProfile>([&](ecs::Entity entity, const components::PlayerProfile& profile) {
        if (profile.id == player) {
            inventory = world_.tryGet<components::Inventory>(entity);
        }
    });
    return inventory;
}

std::optional<ecs::Entity> LogisticsSystem::recruitDrone(PlayerId player, ecs::Entity warehouse)
{
    const auto* building = world_.tryGet<components::Building>(warehouse);
    const auto* footprint = world_.tryGet<components::Footprint>(warehouse);
    if (building == nullptr || building->type != BuildingType::Warehouse || footprint == nullptr
        || !ownedBy(warehouse, player)) {
        return std::nullopt;
    }
    components::Inventory* inventory = playerInventory(player);
    int removedIron = 0;
    int removedElectronics = 0;
    if (!debugMode_) {
        if (inventory == nullptr
            || InventorySystem::amount(*inventory, ItemId::Iron) < components::kTransportDroneIronCost
            || InventorySystem::amount(*inventory, ItemId::Electronics) < components::kTransportDroneElectronicsCost) {
            return std::nullopt;
        }

        removedIron = InventorySystem::remove(*inventory, ItemId::Iron, components::kTransportDroneIronCost);
        removedElectronics =
            InventorySystem::remove(*inventory, ItemId::Electronics, components::kTransportDroneElectronicsCost);
        if (removedIron != components::kTransportDroneIronCost
            || removedElectronics != components::kTransportDroneElectronicsCost) {
            (void)InventorySystem::add(*inventory, ItemId::Iron, removedIron);
            (void)InventorySystem::add(*inventory, ItemId::Electronics, removedElectronics);
            return std::nullopt;
        }
    }

    const GridPos center{footprint->origin.x + footprint->size / 2, footprint->origin.y + footprint->size / 2};
    const auto drone = units_.spawnTransportDrone(player, center);
    if (!drone && !debugMode_) {
        (void)InventorySystem::add(*inventory, ItemId::Iron, removedIron);
        (void)InventorySystem::add(*inventory, ItemId::Electronics, removedElectronics);
    }
    return drone;
}

bool LogisticsSystem::assignIdleDrone(
    PlayerId player, ecs::Entity source, components::LogisticsRouteId routeId)
{
    components::LogisticsRoute* route = findRoute(source, routeId);
    if (route == nullptr || route->source != source || !ownedBy(source, player)
        || !ownedBy(route->destination, player)) {
        return false;
    }

    std::optional<ecs::Entity> selectedDrone;
    world_.each<components::TransportDrone>([&](ecs::Entity entity, const components::TransportDrone& drone) {
        const auto* cargo = world_.tryGet<components::Inventory>(entity);
        const bool hasCargo = cargo != nullptr
            && std::any_of(cargo->slots.begin(), cargo->slots.end(), [](const auto& slot) { return slot.has_value(); });
        if (!drone.routeId && !drone.returnSource && !hasCargo && ownedBy(entity, player)
            && (!selectedDrone || entity < *selectedDrone)) {
            selectedDrone = entity;
        }
    });
    if (!selectedDrone) {
        return false;
    }

    auto* drone = world_.tryGet<components::TransportDrone>(*selectedDrone);
    if (drone == nullptr || !sendToBuilding(*selectedDrone, source)) {
        return false;
    }
    drone->routeId = routeId;
    drone->state = components::DroneTransportState::ToSource;
    ++route->assignedDrones;
    return true;
}

bool LogisticsSystem::unassignDrone(
    PlayerId player, ecs::Entity source, components::LogisticsRouteId routeId)
{
    components::LogisticsRoute* route = findRoute(source, routeId);
    if (route == nullptr || route->source != source || !ownedBy(source, player)
        || !ownedBy(route->destination, player)) {
        return false;
    }

    std::optional<ecs::Entity> assignedDrone;
    world_.each<components::TransportDrone>([&](ecs::Entity entity, const components::TransportDrone& drone) {
        if (drone.routeId == routeId && ownedBy(entity, player)
            && (!assignedDrone || entity < *assignedDrone)) {
            assignedDrone = entity;
        }
    });
    if (!assignedDrone) {
        return false;
    }

    auto* drone = world_.tryGet<components::TransportDrone>(*assignedDrone);
    auto* cargo = world_.tryGet<components::Inventory>(*assignedDrone);
    if (drone == nullptr || cargo == nullptr) {
        return false;
    }

    drone->routeId.reset();
    --route->assignedDrones;
    if (InventorySystem::amount(*cargo, route->item) == 0) {
        drone->state = components::DroneTransportState::Idle;
        return true;
    }

    drone->returnSource = source;
    drone->returnItem = route->item;
    drone->state = components::DroneTransportState::ReturningCargoToSource;
    drone->returnPathPending = !sendToBuilding(*assignedDrone, source);
    return true;
}

int LogisticsSystem::idleDroneCount(PlayerId player) const
{
    int count = 0;
    world_.each<components::TransportDrone>([&](ecs::Entity entity, const components::TransportDrone& drone) {
        if (!drone.routeId && !drone.returnSource && ownedBy(entity, player)) {
            ++count;
        }
    });
    return count;
}

bool LogisticsSystem::supportsBuilding(ecs::Entity building) const
{
    const auto* spec = world_.tryGet<components::Building>(building);
    return spec != nullptr && (spec->type == BuildingType::Warehouse
        || world_.tryGet<components::Production>(building) != nullptr);
}

bool LogisticsSystem::sendToBuilding(ecs::Entity drone, ecs::Entity building)
{
    const auto* footprint = world_.tryGet<components::Footprint>(building);
    if (footprint == nullptr) {
        return false;
    }
    return units_.moveToAdjacent(drone,
        GridPos{footprint->origin.x + footprint->size / 2, footprint->origin.y + footprint->size / 2});
}

void LogisticsSystem::loadCargo(ecs::Entity drone, const components::LogisticsRoute& route)
{
    auto* source = world_.tryGet<components::Inventory>(route.source);
    auto* cargo = world_.tryGet<components::Inventory>(drone);
    if (source == nullptr || cargo == nullptr) {
        return;
    }
    (void)InventorySystem::transfer(*source, *cargo, route.item, InventorySystem::amount(*source, route.item));
}

void LogisticsSystem::unloadCargo(ecs::Entity drone, const components::LogisticsRoute& route)
{
    auto* destination = world_.tryGet<components::Inventory>(route.destination);
    auto* cargo = world_.tryGet<components::Inventory>(drone);
    if (destination == nullptr || cargo == nullptr) {
        return;
    }
    (void)InventorySystem::transfer(*cargo, *destination, route.item, InventorySystem::amount(*cargo, route.item));
}

void LogisticsSystem::advanceDrone(ecs::Entity entity, components::TransportDrone& drone)
{
    if (drone.state == components::DroneTransportState::ReturningCargoToSource) {
        returnCargoToSource(entity, drone);
        return;
    }
    if (!drone.routeId) {
        return;
    }
    const auto source = buildingForRoute(*drone.routeId);
    components::LogisticsRoute* route = source ? findRoute(*source, *drone.routeId) : nullptr;
    if (route == nullptr || !world_.isAlive(route->destination)) {
        detachDrone(entity, drone);
        return;
    }

    const auto* mover = world_.tryGet<components::Mover>(entity);
    if (mover == nullptr || mover->next < mover->path.size()) {
        return;
    }

    auto* cargo = world_.tryGet<components::Inventory>(entity);
    if (cargo == nullptr) {
        detachDrone(entity, drone);
        return;
    }
    const auto cargoAmount = [&cargo, &route]() { return InventorySystem::amount(*cargo, route->item); };

    switch (drone.state) {
    case components::DroneTransportState::Idle:
        return;
    case components::DroneTransportState::ToSource:
    case components::DroneTransportState::ReturningToSource:
    case components::DroneTransportState::WaitingAtSource:
        loadCargo(entity, *route);
        if (cargoAmount() == 0) {
            drone.state = components::DroneTransportState::WaitingAtSource;
            return;
        }
        if (sendToBuilding(entity, route->destination)) {
            drone.state = components::DroneTransportState::ToDestination;
        }
        return;
    case components::DroneTransportState::ToDestination:
    case components::DroneTransportState::WaitingAtDestination:
        unloadCargo(entity, *route);
        if (cargoAmount() > 0) {
            drone.state = components::DroneTransportState::WaitingAtDestination;
            return;
        }
        if (sendToBuilding(entity, route->source)) {
            drone.state = components::DroneTransportState::ReturningToSource;
        }
        return;
    case components::DroneTransportState::ReturningCargoToSource:
        return;
    }
}

void LogisticsSystem::returnCargoToSource(ecs::Entity entity, components::TransportDrone& drone)
{
    if (!drone.returnSource || !drone.returnItem || !world_.isAlive(*drone.returnSource)) {
        drone.returnSource.reset();
        drone.returnItem.reset();
        drone.returnPathPending = false;
        drone.state = components::DroneTransportState::Idle;
        return;
    }
    if (drone.returnPathPending) {
        drone.returnPathPending = !sendToBuilding(entity, *drone.returnSource);
        return;
    }
    const auto* mover = world_.tryGet<components::Mover>(entity);
    if (mover == nullptr) {
        drone.returnSource.reset();
        drone.returnItem.reset();
        drone.returnPathPending = false;
        drone.state = components::DroneTransportState::Idle;
        return;
    }
    if (mover->next < mover->path.size()) {
        return;
    }

    auto* cargo = world_.tryGet<components::Inventory>(entity);
    auto* source = world_.tryGet<components::Inventory>(*drone.returnSource);
    if (cargo == nullptr || source == nullptr) {
        drone.returnSource.reset();
        drone.returnItem.reset();
        drone.returnPathPending = false;
        drone.state = components::DroneTransportState::Idle;
        return;
    }
    (void)InventorySystem::transfer(
        *cargo, *source, *drone.returnItem, InventorySystem::amount(*cargo, *drone.returnItem));
    if (InventorySystem::amount(*cargo, *drone.returnItem) > 0) {
        return;
    }

    drone.returnSource.reset();
    drone.returnItem.reset();
    drone.returnPathPending = false;
    drone.state = components::DroneTransportState::Idle;
}

void LogisticsSystem::detachDrone(ecs::Entity entity, components::TransportDrone& drone)
{
    if (drone.routeId) {
        const auto source = buildingForRoute(*drone.routeId);
        if (source) {
            if (auto* route = findRoute(*source, *drone.routeId); route != nullptr) {
                route->assignedDrones = std::max(0, route->assignedDrones - 1);
            }
        }
    }
    drone.routeId.reset();
    drone.returnSource.reset();
    drone.returnItem.reset();
    drone.returnPathPending = false;
    drone.state = components::DroneTransportState::Idle;
    if (const auto* mover = world_.tryGet<components::Mover>(entity); mover != nullptr) {
        const GridPos destination = mover->destination;
        (void)units_.moveToAdjacent(entity, destination);
    }
}

void LogisticsSystem::update()
{
    std::vector<ecs::Entity> drones;
    world_.each<components::TransportDrone>(
        [&drones](ecs::Entity entity, const components::TransportDrone&) { drones.push_back(entity); });
    for (const ecs::Entity entity : drones) {
        if (auto* drone = world_.tryGet<components::TransportDrone>(entity)) {
            advanceDrone(entity, *drone);
        }
    }
}

} // namespace model
