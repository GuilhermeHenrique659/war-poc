#pragma once

#include "core/EventBus.hpp"
#include "model/GridPos.hpp"
#include "model/Player.hpp"
#include "model/ecs/Entity.hpp"

#include <raylib.h>

#include <optional>
#include <unordered_map>
#include <vector>

namespace controller {

// Map input while no building is selected: click / box selection and right-click move orders.
// Picking uses the unit and building positions announced by the model.
class SelectionInputController {
public:
    explicit SelectionInputController(core::EventBus& bus);

    // overUi: the mouse is over a panel, so no new click starts on the map.
    void update(Vector2 mouse, const Camera2D& camera, bool overUi);
    // Drops an unfinished drag (e.g. a building got selected).
    void cancelDrag();

    [[nodiscard]] bool hasSelection() const { return hasUnitSelection_ || hasBuildingSelection_; }

private:
    struct UnitInfo {
        model::PlayerId owner;
        float x;
        float y;
        int size;
    };

    struct BuildingInfo {
        model::ecs::Entity entity;
        model::PlayerId owner;
        model::GridPos origin;
        int size;
    };

    void subscribe();
    void finishDrag(Vector2 mouse, const Camera2D& camera);
    void selectInBox(Vector2 corner, const Camera2D& camera, bool additive);
    void selectAt(Vector2 mouse, const Camera2D& camera, bool additive);
    void issueMove(Vector2 mouse, const Camera2D& camera);
    void publishBox(Vector2 corner) const;

    [[nodiscard]] std::optional<model::ecs::Entity> unitAt(Vector2 world) const;
    [[nodiscard]] std::optional<model::ecs::Entity> buildingAt(model::GridPos tile) const;

    core::EventBus& bus_;
    std::unordered_map<model::ecs::Entity, UnitInfo> units_;
    std::vector<BuildingInfo> buildings_;
    bool hasUnitSelection_ = false;
    bool hasBuildingSelection_ = false;
    bool dragging_ = false; // Left button went down on the map and is still held.
    bool boxActive_ = false;
    Vector2 dragStart_{0.0f, 0.0f};
};

} // namespace controller
