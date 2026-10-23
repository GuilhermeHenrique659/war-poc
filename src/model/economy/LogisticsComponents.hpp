#pragma once

#include "model/economy/Item.hpp"
#include "model/ecs/Entity.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace model::components {

using LogisticsRouteId = std::uint64_t;

inline constexpr int kTransportDroneIronCost = 50;
inline constexpr int kTransportDroneElectronicsCost = 50;

struct LogisticsRoute {
    LogisticsRouteId id;
    ecs::Entity source;
    ecs::Entity destination;
    ItemId item;
    int assignedDrones = 0;
};

struct LogisticsRoutes {
    std::vector<LogisticsRoute> routes;
};

enum class DroneTransportState {
    Idle,
    ToSource,
    WaitingAtSource,
    ToDestination,
    WaitingAtDestination,
    ReturningToSource,
    ReturningCargoToSource
};

struct TransportDrone {
    std::optional<LogisticsRouteId> routeId;
    std::optional<ecs::Entity> returnSource;
    std::optional<ItemId> returnItem;
    bool returnPathPending = false;
    DroneTransportState state = DroneTransportState::Idle;
};

} // namespace model::components
