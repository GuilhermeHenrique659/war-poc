#pragma once

#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/GridPos.hpp"
#include "model/player/Player.hpp"
#include "view/world/EnvironmentViewModel.hpp"
#include "view/world/WorldObjectsViewModel.hpp"

#include <raylib.h>

#include <optional>

namespace view::gameplay {

// Draws the isometric map, buildings, units, player and placement preview, and owns the camera.
class WorldView {
public:
    explicit WorldView(core::EventBus& bus);

    void draw() const;

private:
    void onPlayerMoved(const events::PlayerMoved& event);
    void onPan(const events::CameraPanRequested& event);
    void focusCameraOn(float x, float y);
    void resetCamera();
    void publishCamera() const;
    [[nodiscard]] Camera2D camera() const;

    void drawGrid(const Camera2D& camera) const;
    void drawDeposits(const Camera2D& camera) const;
    void drawPreview() const;
    void drawSelectionBox() const;

    core::EventBus& bus_;
    EnvironmentViewModel environment_;
    WorldObjectsViewModel objects_;
    Vector2 cameraTarget_{0.0f, 0.0f};
    float cameraZoom_ = 1.0f;
    events::BuildPreviewChanged preview_{};
    events::SelectionBoxChanged selectionBox_{};
    std::optional<events::MapGenerated> worldMap_;
    model::PlayerId localPlayerId_ = model::kNoPlayer;
};

} // namespace view::gameplay
