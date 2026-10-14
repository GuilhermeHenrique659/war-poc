#pragma once

#include "core/EventBus.hpp"
#include "model/BuildingType.hpp"
#include "model/GridPos.hpp"
#include "model/Player.hpp"
#include "model/ecs/Registry.hpp"

#include <optional>
#include <string_view>
#include <unordered_map>

namespace model {

// Building placement rules: selection, preview validity and tile occupancy.
class BuildingSystem {
public:
    BuildingSystem(core::EventBus& bus, ecs::Registry& world);

    // Call after the world is cleared. Publishes the fresh build bar and preview.
    void reset();

    void select(PlayerId player, BuildingType type);
    void cancel(PlayerId player);
    void moveCursor(PlayerId player, std::optional<GridPos> tile);
    void place(PlayerId player, GridPos anchor);

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

    void publishBar(PlayerId player) const;
    void publishPreview(PlayerId player) const;

    core::EventBus& bus_;
    ecs::Registry& world_;
    std::unordered_map<GridPos, ecs::Entity, GridPosHash> occupied_;
    std::unordered_map<PlayerId, BuilderState> builders_;
    std::unordered_map<PlayerId, ecs::Entity> commandCenters_;
};

} // namespace model
