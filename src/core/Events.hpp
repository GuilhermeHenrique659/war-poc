#pragma once

#include "model/build/BuildingType.hpp"
#include "model/economy/Item.hpp"
#include "model/GridPos.hpp"
#include "model/player/Player.hpp"
#include "model/WorldMap.hpp"
#include "model/Scene.hpp"
#include "model/units/UnitType.hpp"
#include "model/ecs/Entity.hpp"

#include <cstddef>
#include <cstdint>
#include <array>
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

// Menu model -> match model
struct StartMatchRequested {
    std::string playerName;
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

struct RecruitDroneRequested {
    model::PlayerId player;
    model::ecs::Entity warehouse;
};

struct AssignDroneToRouteRequested {
    model::PlayerId player;
    model::ecs::Entity source;
    std::uint64_t routeId;
};

struct UnassignDroneFromRouteRequested {
    model::PlayerId player;
    model::ecs::Entity source;
    std::uint64_t routeId;
};

struct TransferInventoryItemRequested {
    model::PlayerId player;
    model::ecs::Entity building;
    std::size_t slotIndex;
    bool fromPlayer;
};

struct TogglePlayerInventoryRequested {
    model::PlayerId player;
};

struct CloseInventoryRequested {
    model::PlayerId player;
};

struct PlayerMoveRequested {
    model::PlayerId player;
    float directionX;
    float directionY;
    float deltaSeconds;
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

struct AttackOrderRequested {
    model::PlayerId player;
    std::vector<model::ecs::Entity> attackers;
    model::ecs::Entity target;
};

// Sends the selected pair to a Cruzador for genealogical combination.
struct CruzadorOrderRequested {
    model::PlayerId player;
    model::ecs::Entity cruzador;
    std::vector<model::ecs::Entity> units;
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

struct PlayerSpawned {
    model::ecs::Entity entity;
    model::PlayerId player;
    float x;
    float y;
    int health;
    int maxHealth;
};

struct PlayerMoved {
    model::ecs::Entity entity;
    model::PlayerId player;
    float x;
    float y;
};

struct PlayerRespawnRequested {
    model::ecs::Entity entity;
};

struct InventorySlotState {
    bool occupied = false;
    model::ItemId item = model::ItemId::Count;
    int currentQuantity = 0;
    int maxQuantity = 0;
};

struct InventoryPanelChanged {
    model::PlayerId player = model::kNoPlayer;
    bool open = false;
    bool playerVisible = false;
    bool hasBuilding = false;
    bool playerInventoryOpen = false;
    model::ecs::Entity building = 0;
    std::string buildingName;
    std::vector<InventorySlotState> buildingSlots;
    std::vector<InventorySlotState> playerSlots;
};

struct InventoryDragChanged {
    model::PlayerId player = model::kNoPlayer;
    bool active = false;
    model::ItemId item = model::ItemId::Count;
    int quantity = 0;
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

struct MapGenerated {
    std::uint32_t seed = 0;
    int width = 0;
    int height = 0;
    model::GridPos origin{};
    std::vector<model::Terrain> terrain;
    std::vector<model::ResourceDeposit> deposits;
};

struct BuildBarEntry {
    model::BuildingType type;
    std::string label;
    int size;
    bool enabled;
    std::string lockedReason;
    std::string cost;
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

struct BuildingRemoved {
    model::ecs::Entity entity;
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

struct DroneSpawned {
    model::ecs::Entity entity;
    model::PlayerId owner;
    float x;
    float y;
};

struct DroneMoved {
    model::ecs::Entity entity;
    float x;
    float y;
};

struct HealthChanged {
    model::ecs::Entity entity;
    int health;
    int maxHealth;
};

struct UnitRemoved {
    model::ecs::Entity entity;
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

struct LogisticsRouteEntry {
    std::uint64_t id = 0;
    model::ecs::Entity destination = 0;
    std::string destinationName;
    model::ItemId item = model::ItemId::Count;
    int assignedDrones = 0;
};

struct LogisticsPanelChanged {
    model::PlayerId player = model::kNoPlayer;
    bool open = false;
    model::ecs::Entity building = 0;
    std::string buildingName;
    bool warehouse = false;
    int idleDrones = 0;
    std::vector<LogisticsRouteEntry> routes;
};

// View -> Controller (needed for mouse picking)
struct CameraChanged {
    float targetX;
    float targetY;
    float zoom;
};

} // namespace events
