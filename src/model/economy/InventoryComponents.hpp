#pragma once

#include "model/economy/Item.hpp"

#include <cstddef>
#include <optional>
#include <vector>

namespace model::components {

struct InventoryItem {
    ItemId item;
    int currentQuantity;
    int maxQuantity;
};

struct Inventory {
    explicit Inventory(std::size_t slotCount = 0) : slots(slotCount) {}

    std::vector<std::optional<InventoryItem>> slots;
};

} // namespace model::components
