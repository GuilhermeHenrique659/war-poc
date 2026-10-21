#pragma once

#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/build/BuildingType.hpp"
#include "model/GridPos.hpp"
#include "model/units/UnitType.hpp"
#include "model/player/Player.hpp"
#include "model/ecs/Entity.hpp"

#include <raylib.h>

#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace view::gameplay {

// Draws the isometric map (infinite grid, buildings, units, player, placement preview) and owns the camera.
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

    struct PlayerVisual {
        model::ecs::Entity entity;
        model::PlayerId player;
        float x;
        float y;
    };

    void subscribeUnits();
    void subscribePlayers();
    void onBuildingPlaced(const events::BuildingPlaced& event);
    void onSelectionChanged(const events::SelectionChanged& event);
    void onPlayerMoved(const events::PlayerMoved& event);
    void onPan(const events::CameraPanRequested& event);
    void focusCameraOn(float x, float y);
    void resetCamera();
    void publishCamera() const;
    [[nodiscard]] Camera2D camera() const;

    void drawGrid(const Camera2D& camera) const;
    // Buildings, units and player are drawn back to front in one painter-order pass.
    void drawObjects() const;
    void drawPreview() const;
    void drawSelectionBox() const;

    core::EventBus& bus_;
    Vector2 cameraTarget_{0.0f, 0.0f};
    float cameraZoom_ = 1.0f;
    std::vector<BuildingVisual> buildings_;
    std::unordered_map<model::ecs::Entity, UnitVisual> units_;
    std::unordered_map<model::ecs::Entity, PlayerVisual> players_;
    std::unordered_set<model::ecs::Entity> selectedUnits_;
    std::optional<model::ecs::Entity> selectedBuilding_;
    events::BuildPreviewChanged preview_{};
    events::SelectionBoxChanged selectionBox_{};
    model::PlayerId localPlayerId_ = model::kNoPlayer;
};

} // namespace view::gameplay
