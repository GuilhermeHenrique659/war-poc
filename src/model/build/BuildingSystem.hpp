#pragma once

#include "core/EventBus.hpp"
#include "model/build/BuildingType.hpp"
#include "model/GridPos.hpp"
#include "model/player/Player.hpp"
#include "model/ecs/Registry.hpp"
#include "model/economy/InventoryComponents.hpp"

#include <functional>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace model {

// Building placement rules: selection, preview validity and tile occupancy.
class BuildingSystem {
public:
    BuildingSystem(core::EventBus& bus, ecs::Registry& world);

    // Call after the world is cleared.
    void reset();
    void initializePlayer(PlayerId player);

    void select(PlayerId player, BuildingType type);
    void cancel(PlayerId player);
    void moveCursor(PlayerId player, std::optional<GridPos> tile);
    void place(PlayerId player, GridPos anchor);
    void remove(ecs::Entity entity);

    // Extra rule for free tiles (e.g. units standing there). Buildings never cover a blocked tile.
    void setTileBlocker(std::function<bool(GridPos)> blocker) { tileBlocker_ = std::move(blocker); }
    void setPlacementRule(std::function<bool(BuildingType, GridPos, int)> rule) { placementRule_ = std::move(rule); }
    // Re-checks preview validity after something other than the cursor changed.
    void refreshPreviews() const;

    [[nodiscard]] bool isOccupied(GridPos tile) const { return occupied_.contains(tile); }
    [[nodiscard]] bool hasBuilding(PlayerId player, BuildingType type) const;

private:
    struct BuilderState {
        std::optional<BuildingType> selected;
        std::optional<GridPos> cursor;
    };

    // Empty when the player may build this type.
    [[nodiscard]] std::string_view lockedReason(PlayerId player, BuildingType type) const;
    [[nodiscard]] bool isUnlocked(PlayerId player, BuildingType type) const;
    [[nodiscard]] bool isAreaFree(GridPos origin, int size) const;
    [[nodiscard]] bool canPlace(PlayerId player, BuildingType type, GridPos origin) const;
    [[nodiscard]] bool hasCommandCenter(PlayerId player) const;
    [[nodiscard]] components::Inventory* playerInventory(PlayerId player) const;
    [[nodiscard]] bool canAfford(PlayerId player, BuildingType type) const;
    bool payCost(PlayerId player, BuildingType type);

    void publishBar(PlayerId player) const;
    void publishPreview(PlayerId player) const;

    core::EventBus& bus_;
    ecs::Registry& world_;
    std::unordered_map<GridPos, ecs::Entity, GridPosHash> occupied_;
    std::unordered_map<PlayerId, BuilderState> builders_;
    std::unordered_map<PlayerId, ecs::Entity> commandCenters_;
    std::function<bool(GridPos)> tileBlocker_;
    std::function<bool(BuildingType, GridPos, int)> placementRule_;
    bool debugMode_ = false;
};

} // namespace model
