#pragma once

#include "model/BuildingType.hpp"
#include "model/GridPos.hpp"
#include "model/Player.hpp"
#include "model/Scene.hpp"
#include "model/UnitType.hpp"
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

struct SettingsNavigateRequested {
    MenuDirection direction;
};

struct SettingsAdjustRequested {
    int delta;
};

struct SettingsNameInputRequested {
    std::string utf8Text;
};

struct SettingsNameBackspaceRequested {};

struct SettingsRowHovered {
    std::size_t index;
};

struct SettingsConfirmRequested {};

struct SettingsBackRequested {};

struct WindowConfigurationRequested {
    int width;
    int height;
    bool fullscreen;
};

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

// Recruits at the player's selected Command Center.
struct RecruitRequested {
    model::PlayerId player;
    model::UnitType type;
};

// Additive keeps the current unit selection and adds to it.
struct SelectUnitsRequested {
    model::PlayerId player;
    std::vector<model::ecs::Entity> units;
    bool additive;
};

struct SelectBuildingRequested {
    model::PlayerId player;
    model::ecs::Entity building;
};

struct SelectionClearRequested {
    model::PlayerId player;
};

// Moves the player's selected units to (around) the tile.
struct MoveOrderRequested {
    model::PlayerId player;
    model::GridPos tile;
};

// Merges all of the player's selected units into one Combined unit.
struct MergeRequested {
    model::PlayerId player;
};

// Controller -> View (selection box being dragged, in screen pixels)
struct SelectionBoxChanged {
    bool visible;
    float x0;
    float y0;
    float x1;
    float y1;
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

// Controller -> View (debug overlay is presentation state)
struct DebugToggleRequested {};

// Any -> Game loop
struct QuitRequested {};

// Model -> View / Controller (state notifications)
struct SceneChanged {
    model::Scene scene;
};

// The player controlled by this client changed. Views and controllers cache this value locally.
struct LocalPlayerChanged {
    model::PlayerId player;
};

struct MenuStateChanged {
    std::vector<std::string> labels;
    std::size_t selectedIndex;
};

struct SettingsStateChanged {
    std::size_t selectedIndex;
    int width;
    int height;
    bool fullscreen;
    std::string playerName;
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
    model::PlayerId player = model::kNoPlayer;
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

// Unit positions are the continuous grid coordinates of the unit's center (tile center = tile + 0.5).
struct UnitSpawned {
    model::ecs::Entity entity;
    model::PlayerId owner;
    model::UnitType type;
    float x;
    float y;
    int health;
    int maxHealth;
    int size; // Square footprint, in tiles per side.
    int baseCount;
};

struct UnitMoved {
    model::ecs::Entity entity;
    float x;
    float y;
};

struct UnitRemoved {
    model::ecs::Entity entity;
};

// Open while the player has two or more units selected. Stats describe the unit the merge would create.
struct MergePanelChanged {
    model::PlayerId player = model::kNoPlayer;
    bool open = false;
    int unitCount = 0;
    bool canMerge = false;
    std::string blockedReason;
    int maxHealth = 0;
    int damage = 0;
    float range = 0.0f;
    float speed = 0.0f;
    int size = 1;

    friend bool operator==(const MergePanelChanged&, const MergePanelChanged&) = default;
};

struct SelectionChanged {
    model::PlayerId player;
    std::vector<model::ecs::Entity> units;
    std::optional<model::ecs::Entity> building;
};

struct RecruitEntry {
    model::UnitType type;
    std::string label;
    int maxHealth;
    int damage;
    float range;
    float speed;
};

// Open while the player has their Command Center selected.
struct RecruitPanelChanged {
    model::PlayerId player;
    bool open;
    std::vector<RecruitEntry> entries;
};

// View -> Controller (needed for mouse picking)
struct CameraChanged {
    float targetX;
    float targetY;
    float zoom;
};

} // namespace events
