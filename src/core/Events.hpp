#pragma once

#include "model/BuildingType.hpp"
#include "model/GridPos.hpp"
#include "model/Player.hpp"
#include "model/Scene.hpp"
#include "model/ecs/Entity.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace events {

// Controller -> Model (commands)
enum class MenuDirection { Up, Down };

struct MenuNavigateRequested {
    MenuDirection direction;
};

struct MenuItemHovered {
    std::size_t index;
};

struct MenuConfirmRequested {};

struct ReturnToMenuRequested {};

// Selecting the already selected building toggles it off.
struct BuildSelectRequested {
    model::PlayerId player;
    model::BuildingType type;
};

struct BuildCancelRequested {
    model::PlayerId player;
};

// Empty tile means the cursor left the map (e.g. it is over the build bar).
struct BuildCursorMoved {
    model::PlayerId player;
    std::optional<model::GridPos> tile;
};

struct BuildPlaceRequested {
    model::PlayerId player;
    model::GridPos tile;
};

// Controller -> View (camera is presentation state)
struct CameraPanRequested {
    float dx; // Screen pixels.
    float dy;
};

struct CameraZoomRequested {
    float steps;
    float screenX; // Zoom pivot, in screen pixels.
    float screenY;
};

// Any -> Game loop
struct QuitRequested {};

// Model -> View / Controller (state notifications)
struct SceneChanged {
    model::Scene scene;
};

struct MenuStateChanged {
    std::vector<std::string> labels;
    std::size_t selectedIndex;
};

// The match world was emptied; drop every cached entity.
struct WorldReset {};

struct BuildBarEntry {
    model::BuildingType type;
    std::string label;
    int size;
    bool enabled;
    std::string lockedReason;
};

struct BuildBarChanged {
    model::PlayerId player;
    std::vector<BuildBarEntry> entries;
    std::optional<model::BuildingType> selected;
};

struct BuildPreviewChanged {
    model::PlayerId player = model::kLocalPlayer;
    bool visible = false;
    model::BuildingType type = model::BuildingType::Wall;
    model::GridPos origin{};
    int size = 1;
    bool valid = false;
};

struct BuildingPlaced {
    model::ecs::Entity entity;
    model::PlayerId owner;
    model::BuildingType type;
    model::GridPos origin;
    int size;
};

// View -> Controller (needed for mouse picking)
struct CameraChanged {
    float targetX;
    float targetY;
    float zoom;
};

} // namespace events
