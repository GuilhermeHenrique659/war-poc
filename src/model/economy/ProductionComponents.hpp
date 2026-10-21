#pragma once

#include "model/economy/Item.hpp"

#include <vector>

namespace model {

struct ProductionInput {
    ItemId item;
    float quantityPerSecond;
};

enum class ProductionType { Primary, Intermediate };

namespace components {

struct Production {
    std::vector<ProductionInput> inputs;
    ItemId output;
    float outputPerSecond;

    [[nodiscard]] ProductionType type() const
    {
        return inputs.empty() ? ProductionType::Primary : ProductionType::Intermediate;
    }
};

} // namespace components
} // namespace model
