#pragma once

#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/GridPos.hpp"
#include "model/build/BuildingType.hpp"
#include "model/economy/LogisticsComponents.hpp"
#include "model/player/Player.hpp"
#include "model/ecs/Registry.hpp"
#include "model/units/UnitSystem.hpp"

#include <optional>

namespace model {

class LogisticsSystem {
public:
    LogisticsSystem(core::EventBus& bus, ecs::Registry& world, UnitSystem& units);

    void reset();
    void update();
    [[nodiscard]] std::optional<ecs::Entity> recruitDrone(PlayerId player, ecs::Entity warehouse);
    [[nodiscard]] bool assignIdleDrone(PlayerId player, ecs::Entity source, components::LogisticsRouteId routeId);
    [[nodiscard]] bool unassignDrone(PlayerId player, ecs::Entity source, components::LogisticsRouteId routeId);
    [[nodiscard]] int idleDroneCount(PlayerId player) const;
    [[nodiscard]] bool supportsBuilding(ecs::Entity building) const;

private:
    void onBuildingPlaced(const events::BuildingPlaced& event);
    void onBuildingRemoved(const events::BuildingRemoved& event);
    void createRoute(ecs::Entity source, ecs::Entity destination, ItemId item);
    [[nodiscard]] components::LogisticsRoute* findRoute(
        ecs::Entity source, components::LogisticsRouteId routeId) const;
    [[nodiscard]] std::optional<ecs::Entity> buildingForRoute(components::LogisticsRouteId routeId) const;
    [[nodiscard]] bool ownedBy(ecs::Entity entity, PlayerId player) const;
    [[nodiscard]] bool sendToBuilding(ecs::Entity drone, ecs::Entity building);
    void loadCargo(ecs::Entity drone, const components::LogisticsRoute& route);
    void unloadCargo(ecs::Entity drone, const components::LogisticsRoute& route);
    void returnCargoToSource(ecs::Entity drone, components::TransportDrone& transport);
    void advanceDrone(ecs::Entity drone, components::TransportDrone& transport);
    void detachDrone(ecs::Entity drone, components::TransportDrone& transport);

    core::EventBus& bus_;
    ecs::Registry& world_;
    UnitSystem& units_;
    components::LogisticsRouteId nextRouteId_ = 1;
};

} // namespace model
