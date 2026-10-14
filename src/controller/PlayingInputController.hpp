#pragma once

#include "core/EventBus.hpp"
#include "model/BuildingType.hpp"
#include "model/GridPos.hpp"

#include <raylib.h>

#include <cstddef>
#include <optional>
#include <vector>

namespace controller {

// Match input: camera movement, build bar selection and placing buildings on the grid.
class PlayingInputController {
public:
    explicit PlayingInputController(core::EventBus& bus);

    void update();

private:
    // Returns true when the input was consumed.
    bool handleCancel();
    void handleCamera();
    void handleShortcuts();
    void handleBarMouse(Vector2 mouse);
    void handleMapMouse(Vector2 mouse);
    void publishCursor(std::optional<model::GridPos> tile);
    [[nodiscard]] std::optional<std::size_t> slotUnderMouse(Vector2 mouse) const;

    core::EventBus& bus_;
    Vector2 cameraTarget_{0.0f, 0.0f};
    float cameraZoom_ = 1.0f;
    std::vector<model::BuildingType> barTypes_;
    bool hasSelection_ = false;
    std::optional<model::GridPos> lastTile_;
    bool placing_ = false; // Left button went down on the map and is still held.
};

} // namespace controller
