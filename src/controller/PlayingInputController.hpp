#pragma once

#include "controller/SelectionInputController.hpp"
#include "core/EventBus.hpp"
#include "model/BuildingType.hpp"
#include "model/GridPos.hpp"
#include "model/UnitType.hpp"
#include "model/Player.hpp"

#include <raylib.h>

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

namespace controller {

// Match input: camera, build bar, placing buildings, recruit panel and unit selection / orders.
class PlayingInputController {
public:
    explicit PlayingInputController(core::EventBus& bus);

    void update();

private:
    // Returns true when the input was consumed.
    bool handleCancel();
    void handlePlayerMovement();
    void handleShortcuts();
    void handleBarMouse(Vector2 mouse);
    void handlePanelMouse(Vector2 mouse);
    void handleMapMouse(Vector2 mouse, const Camera2D& camera);
    void publishCursor(std::optional<model::GridPos> tile);
    [[nodiscard]] std::optional<std::size_t> slotUnderMouse(Vector2 mouse) const;
    [[nodiscard]] bool isOverPanel(Vector2 mouse) const;

    core::EventBus& bus_;
    std::unique_ptr<SelectionInputController> selection_;
    Vector2 cameraTarget_{0.0f, 0.0f};
    float cameraZoom_ = 1.0f;
    std::vector<model::BuildingType> barTypes_;
    bool hasBuildSelection_ = false;
    std::optional<model::GridPos> lastTile_;
    bool placing_ = false; // Left button went down on the map and is still held.
    std::vector<model::UnitType> panelTypes_; // Empty while the recruit panel is closed.
    bool mergeOpen_ = false;
    model::PlayerId localPlayerId_ = model::kNoPlayer;
};

} // namespace controller
