#include "view/DebugOverlay.hpp"

#include "view/IsoProjection.hpp"
#include "view/units/UnitLayout.hpp"
#include "view/ui/GameText.hpp"
#include "view/ui/Shapes.hpp"
#include "view/ui/Text.hpp"
#include "view/ui/TextCatalog.hpp"

#include <array>
#include <string>
#include <string_view>

namespace view {

namespace {

constexpr int kFontSize = 10;
constexpr int kStatsFontSize = 16;
constexpr int kPadding = 4;
constexpr Color kPanelColor{0, 0, 0, 170};
constexpr Color kFootprintColor{0, 230, 255, 255};
constexpr Color kHitBoxColor{255, 220, 0, 255};
constexpr Color kPositionColor{255, 60, 200, 255};

// Text over a dark backdrop, centered horizontally on `x` with its bottom at `bottom`.
void drawLabel(std::string_view value, float x, float bottom)
{
    const ui::Text text{ui::TextStyle{kFontSize, RAYWHITE}};
    const float width = static_cast<float>(text.width(value));
    const float left = x - width / 2.0F;
    const float top = bottom - static_cast<float>(kFontSize) - static_cast<float>(kPadding);
    ui::fill(Rectangle{left - 2.0F, top - 1.0F, width + 4.0F, static_cast<float>(kFontSize) + 2.0F}, kPanelColor);
    text.draw(Rectangle{left, top, width, static_cast<float>(kFontSize)}, value);
}

// Outline of a footprint on the ground, drawn in screen space so it stays 1 px at any zoom.
void drawFootprintOutline(const Camera2D& camera, float x, float y, float size)
{
    const std::array corners{
        GetWorldToScreen2D(iso::gridToWorld(x, y), camera),
        GetWorldToScreen2D(iso::gridToWorld(x + size, y), camera),
        GetWorldToScreen2D(iso::gridToWorld(x + size, y + size), camera),
        GetWorldToScreen2D(iso::gridToWorld(x, y + size), camera),
    };
    for (std::size_t i = 0; i < corners.size(); ++i) {
        ui::line(corners[i], corners[(i + 1) % corners.size()], kFootprintColor);
    }
}

[[nodiscard]] Color fpsColor(int fps)
{
    if (fps >= 55) {
        return GREEN;
    }
    return fps >= 30 ? YELLOW : RED;
}

} // namespace

DebugOverlay::DebugOverlay(core::EventBus& bus)
{
    bus.subscribe<events::DebugToggleRequested>([this](const events::DebugToggleRequested&) { enabled_ = !enabled_; });
    bus.subscribe<events::SceneChanged>([this](const events::SceneChanged& e) { scene_ = e.scene; });
    bus.subscribe<events::CameraChanged>([this](const events::CameraChanged& e) {
        cameraTarget_ = Vector2{e.targetX, e.targetY};
        cameraZoom_ = e.zoom;
    });
    subscribeWorld(bus);
}

void DebugOverlay::subscribeWorld(core::EventBus& bus)
{
    bus.subscribe<events::WorldReset>([this](const events::WorldReset&) {
        buildings_.clear();
        units_.clear();
    });
    bus.subscribe<events::BuildingPlaced>([this](const events::BuildingPlaced& e) {
        buildings_[e.entity] = BuildingInfo{e.type, e.origin, e.size};
    });
    bus.subscribe<events::ConstructionStarted>([this](const events::ConstructionStarted& e) {
        buildings_[e.entity] = BuildingInfo{e.type, e.origin, e.size};
    });
    bus.subscribe<events::UnitSpawned>(
        [this](const events::UnitSpawned& e) { units_[e.entity] = UnitInfo{e.type, e.x, e.y, e.size}; });
    bus.subscribe<events::UnitRemoved>([this](const events::UnitRemoved& e) { units_.erase(e.entity); });
    bus.subscribe<events::UnitMoved>([this](const events::UnitMoved& e) {
        const auto it = units_.find(e.entity);
        if (it == units_.end()) {
            return;
        }
        it->second.x = e.x;
        it->second.y = e.y;
    });
}

void DebugOverlay::draw() const
{
    if (!enabled_) {
        return;
    }

    const Camera2D cam = camera();
    if (scene_ == model::Scene::Playing) {
        drawBuildings(cam);
        drawUnits(cam);
    }
    drawStats(cam);
}

Camera2D DebugOverlay::camera() const
{
    return iso::makeCamera(cameraTarget_, cameraZoom_, GetScreenWidth(), GetScreenHeight());
}

void DebugOverlay::drawStats(const Camera2D& cam) const
{
    constexpr int kLineHeight = kStatsFontSize + 4;
    constexpr int kLeft = 10;
    constexpr int kTop = 10;
    constexpr int kWidth = 250;

    const bool playing = scene_ == model::Scene::Playing;
    const int lines = playing ? 5 : 2;
    ui::fill(Rectangle{static_cast<float>(kLeft - kPadding), static_cast<float>(kTop - kPadding),
                 static_cast<float>(kWidth), static_cast<float>(lines * kLineHeight + kPadding)},
        kPanelColor);

    const auto drawLine = [](int line, const std::string& value, Color color) {
        ui::Text{ui::TextStyle{kStatsFontSize, color}}.draw(
            Rectangle{static_cast<float>(kLeft), static_cast<float>(kTop + line * kLineHeight),
                static_cast<float>(kWidth), static_cast<float>(kStatsFontSize)},
            value);
    };

    const int fps = GetFPS();
    const ui::Text fpsText{ui::TextStyle{kStatsFontSize, fpsColor(fps)}};
    fpsText.draw(Rectangle{static_cast<float>(kLeft), static_cast<float>(kTop), static_cast<float>(kWidth),
                    static_cast<float>(kStatsFontSize)},
        ui::format(ui::TextId::DebugFps, fps, static_cast<double>(GetFrameTime() * 1000.0F)));

    int line = 1;
    drawLine(line, ui::format(ui::TextId::DebugScene, ui::sceneName(scene_)), RAYWHITE);
    if (!playing) {
        return;
    }

    ++line;
    drawLine(line,
        ui::format(ui::TextId::DebugCounts, static_cast<int>(buildings_.size()), static_cast<int>(units_.size())),
        RAYWHITE);
    ++line;
    drawLine(line,
        ui::format(ui::TextId::DebugCamera, static_cast<double>(cameraTarget_.x),
            static_cast<double>(cameraTarget_.y), static_cast<double>(cameraZoom_)),
        RAYWHITE);
    ++line;
    const model::GridPos tile = iso::screenToGrid(GetMousePosition(), cam);
    drawLine(line, ui::format(ui::TextId::DebugMouse, tile.x, tile.y), RAYWHITE);
}

void DebugOverlay::drawBuildings(const Camera2D& cam) const
{
    for (const auto& [entity, building] : buildings_) {
        const float x = static_cast<float>(building.origin.x);
        const float y = static_cast<float>(building.origin.y);
        const float size = static_cast<float>(building.size);
        drawFootprintOutline(cam, x, y, size);

        const Vector2 center = GetWorldToScreen2D(iso::gridToWorld(x + size / 2.0f, y + size / 2.0f), cam);
        DrawCircleV(center, 2.0f, kPositionColor);
        drawLabel(ui::format(ui::TextId::DebugBuilding, static_cast<unsigned>(entity),
                      ui::buildingName(building.type), building.origin.x, building.origin.y, building.size,
                      building.size),
            center.x, center.y);
    }
}

void DebugOverlay::drawUnits(const Camera2D& cam) const
{
    for (const auto& [entity, unit] : units_) {
        const float size = static_cast<float>(unit.size);
        drawFootprintOutline(cam, unit.x - size / 2.0f, unit.y - size / 2.0f, size);

        const Rectangle hit = unit_layout::hitBounds(unit.x, unit.y, unit.size);
        const Vector2 topLeft = GetWorldToScreen2D(Vector2{hit.x, hit.y}, cam);
        const Rectangle screenHit{topLeft.x, topLeft.y, hit.width * cam.zoom, hit.height * cam.zoom};
        ui::outline(screenHit, kHitBoxColor);

        const Vector2 ground = GetWorldToScreen2D(unit_layout::groundPoint(unit.x, unit.y), cam);
        DrawCircleV(ground, 2.0f, kPositionColor);
        drawLabel(ui::format(ui::TextId::DebugUnit, static_cast<unsigned>(entity), ui::unitName(unit.type),
                      static_cast<double>(unit.x), static_cast<double>(unit.y)),
            screenHit.x + screenHit.width / 2.0f, screenHit.y);
    }
}

} // namespace view
