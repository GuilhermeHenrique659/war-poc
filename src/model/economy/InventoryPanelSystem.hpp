#pragma once

#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/player/PlayerAvatarSystem.hpp"
#include "model/selection/SelectionSystem.hpp"
#include "model/ecs/Registry.hpp"

#include <cstddef>
#include <unordered_map>
#include <unordered_set>

namespace model {

// Owns which inventories are on screen and publishes them as generic panes.
// Player, building and units all become the same InventoryPaneState.
class InventoryPanelSystem {
public:
    InventoryPanelSystem(core::EventBus& bus, ecs::Registry& world, SelectionSystem& selection,
        PlayerAvatarSystem& players);

    // Call after the world is cleared.
    void reset();
    // Republishes open panels, so production, ammo and transfers stay up to date.
    void update() const;
    // Moves a stack between the player and the selected building pane.
    // Returns true when something was transferred.
    bool transfer(PlayerId player, ecs::Entity building, std::size_t slotIndex, bool fromPlayer);

private:
    void onSelectionChanged(const events::SelectionChanged& event);
    void publish(PlayerId player) const;

    core::EventBus& bus_;
    ecs::Registry& world_;
    SelectionSystem& selection_;
    PlayerAvatarSystem& players_;
    std::unordered_map<PlayerId, ecs::Entity> buildingPanels_;
    std::unordered_set<PlayerId> unitPanels_;
    std::unordered_map<PlayerId, bool> playerInventoryOpen_;
};

} // namespace model
