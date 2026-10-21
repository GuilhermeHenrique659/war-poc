#pragma once

#include "model/ecs/Registry.hpp"

#include <unordered_map>
#include <vector>

namespace model {

class ProductionSystem {
public:
    void update(ecs::Registry& world, float deltaSeconds);
    void reset();

private:
    struct Progress {
        std::vector<float> inputRemainders;
        float outputRemainder = 0.0F;
    };

    std::unordered_map<ecs::Entity, Progress> progress_;
};

} // namespace model
