#include "model/economy/InventorySystem.hpp"

#include <algorithm>

namespace model {
namespace {

[[nodiscard]] bool validItem(ItemId item)
{
    return itemDefinition(item).maxStack > 0;
}

[[nodiscard]] bool validQuantity(int quantity)
{
    return quantity > 0;
}

} // namespace

int InventorySystem::amount(const components::Inventory& inventory, ItemId item)
{
    int total = 0;
    for (const auto& slot : inventory.slots) {
        if (slot && slot->item == item) {
            total += slot->currentQuantity;
        }
    }
    return total;
}

int InventorySystem::availableCapacity(const components::Inventory& inventory, ItemId item)
{
    if (!validItem(item)) {
        return 0;
    }

    int capacity = 0;
    const int maxStack = itemDefinition(item).maxStack;
    for (const auto& slot : inventory.slots) {
        if (!slot) {
            capacity += maxStack;
        } else if (slot->item == item) {
            capacity += std::max(0, slot->maxQuantity - slot->currentQuantity);
        }
    }
    return capacity;
}

int InventorySystem::add(components::Inventory& inventory, ItemId item, int quantity)
{
    if (!validItem(item) || !validQuantity(quantity)) {
        return 0;
    }

    int remaining = std::min(quantity, availableCapacity(inventory, item));
    const int accepted = remaining;
    for (auto& slot : inventory.slots) {
        if (!slot || slot->item != item) {
            continue;
        }
        const int moved = std::min(remaining, slot->maxQuantity - slot->currentQuantity);
        slot->currentQuantity += moved;
        remaining -= moved;
        if (remaining == 0) {
            return accepted;
        }
    }

    const int maxStack = itemDefinition(item).maxStack;
    for (auto& slot : inventory.slots) {
        if (slot) {
            continue;
        }
        const int moved = std::min(remaining, maxStack);
        slot = components::InventoryItem{item, moved, maxStack};
        remaining -= moved;
        if (remaining == 0) {
            break;
        }
    }
    return accepted - remaining;
}

int InventorySystem::remove(components::Inventory& inventory, ItemId item, int quantity)
{
    if (!validItem(item) || !validQuantity(quantity)) {
        return 0;
    }

    int remaining = std::min(quantity, amount(inventory, item));
    const int removed = remaining;
    for (auto& slot : inventory.slots) {
        if (!slot || slot->item != item) {
            continue;
        }
        const int taken = std::min(remaining, slot->currentQuantity);
        slot->currentQuantity -= taken;
        remaining -= taken;
        if (slot->currentQuantity == 0) {
            slot.reset();
        }
        if (remaining == 0) {
            break;
        }
    }
    return removed - remaining;
}

int InventorySystem::transfer(
    components::Inventory& source, components::Inventory& destination, ItemId item, int quantity)
{
    if (&source == &destination || !validQuantity(quantity)) {
        return 0;
    }
    const int moved = add(destination, item, std::min(quantity, amount(source, item)));
    if (moved == 0) {
        return 0;
    }
    const int removed = remove(source, item, moved);
    if (removed == moved) {
        return moved;
    }
    (void)add(source, item, moved - removed);
    return removed;
}

} // namespace model
