#pragma once

#include "core/EventBus.hpp"
#include "model/economy/ProductionComponents.hpp"
#include "model/ecs/Registry.hpp"

#include <unordered_map>
#include <vector>

namespace model {

// Turns inputs into outputs. Reports every whole unit consumed or produced to the economy, and
// keeps the producing building's state in sync with whether it is actually converting.
class ProductionSystem {
public:
    explicit ProductionSystem(core::EventBus& bus);

    void update(ecs::Registry& world, float deltaSeconds);
    void reset();

private:
    struct Progress {
        std::vector<float> inputRemainders;
        float outputRemainder = 0.0F;
    };

    // Converts for this tick. Returns false when the producer has nothing to convert.
    bool produce(ecs::Registry& world, ecs::Entity entity, const components::Production& production,
        float deltaSeconds);

    core::EventBus& bus_;
    std::unordered_map<ecs::Entity, Progress> progress_;
};

} // namespace model
