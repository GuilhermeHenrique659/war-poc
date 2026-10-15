#pragma once

#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/GridPos.hpp"
#include "model/Player.hpp"
#include "model/SelectionSystem.hpp"
#include "model/UnitSystem.hpp"
#include "model/UnitType.hpp"
#include "model/ecs/Registry.hpp"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace model {

// Merging the selected units into one Combined unit, and the merge panel that previews it.
class MergeSystem {
public:
    MergeSystem(core::EventBus& bus, ecs::Registry& world, UnitSystem& units, SelectionSystem& selection);

    // Call after the world is cleared.
    void reset();
    void initializePlayer(PlayerId player);

    // Republishes the player's merge panel if it changed (selection or positions changed).
    void refresh(PlayerId player);
    void refreshAll();

    void merge(PlayerId player);

private:
    struct Plan {
        std::vector<ecs::Entity> units;
        UnitStats result{};
        std::optional<GridPos> origin;
        std::string blockedReason; // Empty when the merge can happen.
    };

    [[nodiscard]] Plan plan(PlayerId player) const;

    core::EventBus& bus_;
    ecs::Registry& world_;
    UnitSystem& units_;
    SelectionSystem& selection_;
    std::unordered_map<PlayerId, events::MergePanelChanged> published_;
};

} // namespace model
