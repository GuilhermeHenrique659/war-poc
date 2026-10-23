#pragma once

#include "model/economy/Item.hpp"

#include <vector>

namespace model {

struct ProductionInput {
    ItemId item;
    float quantityPerSecond;
};

namespace components {

struct Production {
    std::vector<ProductionInput> inputs;
    ItemId output;
    float outputPerSecond;
};

} // namespace components
} // namespace model
