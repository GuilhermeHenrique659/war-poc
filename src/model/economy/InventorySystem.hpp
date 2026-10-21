#pragma once

#include "model/economy/InventoryComponents.hpp"

namespace model {

class InventorySystem {
public:
    [[nodiscard]] static int amount(const components::Inventory& inventory, ItemId item);
    [[nodiscard]] static int availableCapacity(const components::Inventory& inventory, ItemId item);
    static int add(components::Inventory& inventory, ItemId item, int quantity);
    static int remove(components::Inventory& inventory, ItemId item, int quantity);
    static int transfer(components::Inventory& source, components::Inventory& destination, ItemId item, int quantity);
};

} // namespace model
