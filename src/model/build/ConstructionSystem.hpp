#pragma once

#include "core/EventBus.hpp"
#include "model/build/ConstructionComponents.hpp"
#include "model/ecs/Entity.hpp"
#include "model/ecs/Registry.hpp"

namespace model {

// Advances construction sites: the nominal duration ticks while the site has the proportional
// materials in its own inventory. Without material the progress pauses. Debug mode ignores them.
class ConstructionSystem {
public:
    ConstructionSystem(core::EventBus& bus, ecs::Registry& world);

    // Call after the world is cleared.
    void reset();
    void update(float deltaSeconds);

private:
    void updateSite(ecs::Entity entity, components::ConstructionSite& site, float deltaSeconds);
    void completeSite(ecs::Entity entity, const components::ConstructionSite& site);

    core::EventBus& bus_;
    ecs::Registry& world_;
    bool debugMode_ = false;
};

} // namespace model
