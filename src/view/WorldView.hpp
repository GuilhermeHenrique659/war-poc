#pragma once

#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/BuildingType.hpp"
#include "model/GridPos.hpp"
#include "model/ecs/Entity.hpp"

#include <raylib.h>

#include <vector>

namespace view {

// Draws the isometric map (infinite grid, buildings, placement preview) and owns the camera.
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

    void onBuildingPlaced(const events::BuildingPlaced& event);
    void onPan(const events::CameraPanRequested& event);
    void onZoom(const events::CameraZoomRequested& event);
    void resetCamera();
    void publishCamera() const;
    [[nodiscard]] Camera2D camera() const;

    void drawGrid(const Camera2D& camera) const;
    void drawBuildings() const;
    void drawPreview() const;

    core::EventBus& bus_;
    Vector2 cameraTarget_{0.0f, 0.0f};
    float cameraZoom_ = 1.0f;
    std::vector<BuildingVisual> buildings_; // Kept sorted back to front.
    events::BuildPreviewChanged preview_{};
};

} // namespace view
