#pragma once

#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/GridPos.hpp"
#include "model/build/ConstructionComponents.hpp"
#include "model/ecs/Entity.hpp"
#include "model/ecs/Registry.hpp"
#include "model/economy/InventoryComponents.hpp"
#include "model/player/Player.hpp"
#include "model/units/UnitSystem.hpp"

#include <optional>
#include <unordered_set>

namespace model {

// Tasks construction drones: they fetch missing materials from the nearest Warehouse (or Command
// Center) and deliver them to construction sites. One drone is granted for free with the Command
// Center; more can be recruited there.
class ConstructionDroneSystem {
public:
    ConstructionDroneSystem(core::EventBus& bus, ecs::Registry& world, UnitSystem& units);

    // Call after the world is cleared.
    void reset();
    void update();

    // Pays the Command Center cost and spawns a construction drone next to it.
    bool recruit(PlayerId player);

private:
    struct TaskCandidate {
        ecs::Entity site = 0;
        ecs::Entity source = 0;
        ItemId item = ItemId::Count;
        float ratio = 0.0F;
    };

    void onBuildingPlaced(const events::BuildingPlaced& event);
    void advanceDrone(ecs::Entity entity, components::ConstructionDrone& drone);
    void chooseTask(ecs::Entity entity, components::ConstructionDrone& drone, PlayerId player);
    void advanceToSource(
        ecs::Entity entity, components::ConstructionDrone& drone, components::Inventory& cargo);
    void advanceToSite(
        ecs::Entity entity, components::ConstructionDrone& drone, components::Inventory& cargo);

    [[nodiscard]] std::optional<ecs::Entity> commandCenter(PlayerId player) const;
    [[nodiscard]] std::optional<ecs::Entity> nearestSource(
        PlayerId player, ItemId item, GridPos target, ecs::Entity site) const;
    [[nodiscard]] std::optional<ecs::Entity> nearestSiteNeeding(
        PlayerId player, ItemId item, ecs::Entity drone) const;
    [[nodiscard]] bool sendTo(ecs::Entity drone, ecs::Entity building) const;
    void stopTask(components::ConstructionDrone& drone, const components::Inventory& cargo) const;

    core::EventBus& bus_;
    ecs::Registry& world_;
    UnitSystem& units_;
    std::unordered_set<PlayerId> granted_;
    bool debugMode_ = false;
};

} // namespace model
