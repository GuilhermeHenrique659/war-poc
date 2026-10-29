#include "model/StateNotifier.hpp"

#include "core/Events.hpp"

namespace model {

namespace {

template <typename State, typename Changed>
void notify(core::EventBus& bus, ecs::Registry& world, ecs::Entity entity, State state)
{
    auto* current = world.tryGet<State>(entity);
    if (current == nullptr) {
        world.emplace<State>(entity, state);
        bus.publish(Changed{entity, state});
        return;
    }
    if (*current == state) {
        return;
    }
    *current = state;
    bus.publish(Changed{entity, state});
}

} // namespace

void setUnitState(core::EventBus& bus, ecs::Registry& world, ecs::Entity entity, UnitState state)
{
    notify<UnitState, events::UnitStateChanged>(bus, world, entity, state);
}

void setPlayerState(core::EventBus& bus, ecs::Registry& world, ecs::Entity entity, PlayerState state)
{
    notify<PlayerState, events::PlayerStateChanged>(bus, world, entity, state);
}

void setBuildingState(core::EventBus& bus, ecs::Registry& world, ecs::Entity entity, BuildingState state)
{
    notify<BuildingState, events::BuildingStateChanged>(bus, world, entity, state);
}

} // namespace model
