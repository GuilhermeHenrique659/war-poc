#pragma once

#include "core/EventBus.hpp"
#include "model/ecs/Registry.hpp"

#include <unordered_map>
#include <vector>

namespace model {

// Turns inputs into outputs. Reports every whole unit consumed or produced to the economy.
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

    core::EventBus& bus_;
    std::unordered_map<ecs::Entity, Progress> progress_;
};

} // namespace model
