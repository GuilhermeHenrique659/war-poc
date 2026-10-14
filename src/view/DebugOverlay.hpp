#pragma once

#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/BuildingType.hpp"
#include "model/GridPos.hpp"
#include "model/Scene.hpp"
#include "model/UnitType.hpp"
#include "model/ecs/Entity.hpp"

#include <raylib.h>

#include <unordered_map>

namespace view {

// Toggled with F3: FPS counter and, during a match, every entity's box and position.
class DebugOverlay {
public:
    explicit DebugOverlay(core::EventBus& bus);

    void draw() const;

private:
    struct BuildingInfo {
        model::BuildingType type;
        model::GridPos origin;
        int size;
    };

    struct UnitInfo {
        model::UnitType type;
        float x;
        float y;
        int size;
    };

    void subscribeWorld(core::EventBus& bus);
    [[nodiscard]] Camera2D camera() const;

    void drawStats(const Camera2D& camera) const;
    void drawBuildings(const Camera2D& camera) const;
    void drawUnits(const Camera2D& camera) const;

    bool enabled_ = false;
    model::Scene scene_ = model::Scene::MainMenu;
    Vector2 cameraTarget_{0.0f, 0.0f};
    float cameraZoom_ = 1.0f;
    std::unordered_map<model::ecs::Entity, BuildingInfo> buildings_;
    std::unordered_map<model::ecs::Entity, UnitInfo> units_;
};

} // namespace view
