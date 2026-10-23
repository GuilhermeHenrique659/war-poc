#pragma once

#include "model/economy/InventoryComponents.hpp"
#include "model/ecs/Registry.hpp"
#include "model/player/Player.hpp"
#include "model/player/PlayerComponents.hpp"

namespace model {

// Inventory held by the player's avatar entity, or nullptr when the player has no avatar yet.
[[nodiscard]] inline components::Inventory* playerInventory(ecs::Registry& world, PlayerId player)
{
    components::Inventory* inventory = nullptr;
    world.each<components::PlayerProfile>([&](ecs::Entity entity, const components::PlayerProfile& profile) {
        if (profile.id == player) {
            inventory = world.tryGet<components::Inventory>(entity);
        }
    });
    return inventory;
}

} // namespace model
