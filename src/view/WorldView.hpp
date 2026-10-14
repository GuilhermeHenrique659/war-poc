#pragma once

#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/BuildingType.hpp"
#include "model/GridPos.hpp"
#include "model/UnitType.hpp"
#include "model/ecs/Entity.hpp"

#include <raylib.h>

#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace view {

// Draws the isometric map (infinite grid, buildings, units, placement preview, selection) and owns the camera.
class WorldView {
public:
    explicit WorldView(core::EventBus& bus);

    void draw() const;

private:
    struct BuildingVisual {
        model::ecs::Entity entity;
        model::BuildingType type;
        model::GridPos origin;
        int size;
    };

    struct UnitVisual {
        model::ecs::Entity entity;
        model::UnitType type;
        float x;
        float y;
        int health;
        int maxHealth;
        int size;
    };

    void subscribeUnits();
    void onBuildingPlaced(const events::BuildingPlaced& event);
    void onSelectionChanged(const events::SelectionChanged& event);
    void onPan(const events::CameraPanRequested& event);
    void onZoom(const events::CameraZoomRequested& event);
    void resetCamera();
    void publishCamera() const;
    [[nodiscard]] Camera2D camera() const;

    void drawGrid(const Camera2D& camera) const;
    void drawSelectedBuilding() const;
    // Buildings and units together, back to front.
    void drawObjects() const;
    void drawHealthBars() const;
    void drawPreview() const;
    void drawSelectionBox() const;

    core::EventBus& bus_;
    Vector2 cameraTarget_{0.0f, 0.0f};
    float cameraZoom_ = 1.0f;
    std::vector<BuildingVisual> buildings_; // Kept sorted back to front.
    std::unordered_map<model::ecs::Entity, UnitVisual> units_;
    std::unordered_set<model::ecs::Entity> selectedUnits_;
    std::optional<model::ecs::Entity> selectedBuilding_;
    events::BuildPreviewChanged preview_{};
    events::SelectionBoxChanged selectionBox_{};
};

} // namespace view
