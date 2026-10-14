#include "view/DebugOverlay.hpp"

#include "view/IsoProjection.hpp"
#include "view/UnitLayout.hpp"

#include <array>

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
void drawLabel(const char* text, float x, float bottom)
{
    const int width = MeasureText(text, kFontSize);
    const int left = static_cast<int>(x) - width / 2;
    const int top = static_cast<int>(bottom) - kFontSize - kPadding;
    DrawRectangle(left - 2, top - 1, width + 4, kFontSize + 2, kPanelColor);
    DrawText(text, left, top, kFontSize, RAYWHITE);
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
        DrawLineV(corners[i], corners[(i + 1) % corners.size()], kFootprintColor);
    }
}

[[nodiscard]] Color fpsColor(int fps)
{
    if (fps >= 55) {
        return GREEN;
    }
    return fps >= 30 ? YELLOW : RED;
}

[[nodiscard]] const char* sceneLabel(model::Scene scene)
{
    switch (scene) {
    case model::Scene::MainMenu:
        return "Menu";
    case model::Scene::Playing:
        return "Partida";
    }
    return "?";
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
    DrawRectangle(kLeft - kPadding, kTop - kPadding, kWidth, lines * kLineHeight + kPadding, kPanelColor);

    const int fps = GetFPS();
    int y = kTop;
    DrawText(TextFormat("FPS: %d (%.2f ms)", fps, static_cast<double>(GetFrameTime() * 1000.0f)), kLeft, y,
        kStatsFontSize, fpsColor(fps));
    y += kLineHeight;
    DrawText(TextFormat("Cena: %s", sceneLabel(scene_)), kLeft, y, kStatsFontSize, RAYWHITE);
    if (!playing) {
        return;
    }

    y += kLineHeight;
    DrawText(TextFormat("Construcoes: %zu  Tropas: %zu", buildings_.size(), units_.size()), kLeft, y,
        kStatsFontSize, RAYWHITE);
    y += kLineHeight;
    DrawText(TextFormat("Camera: (%.0f, %.0f) zoom %.2f", static_cast<double>(cameraTarget_.x),
                 static_cast<double>(cameraTarget_.y), static_cast<double>(cameraZoom_)),
        kLeft, y, kStatsFontSize, RAYWHITE);
    y += kLineHeight;
    const model::GridPos tile = iso::screenToGrid(GetMousePosition(), cam);
    DrawText(TextFormat("Mouse: tile (%d, %d)", tile.x, tile.y), kLeft, y, kStatsFontSize, RAYWHITE);
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
        drawLabel(TextFormat("#%u %s (%d, %d) %dx%d", static_cast<unsigned>(entity),
                      model::buildingSpec(building.type).label.data(), building.origin.x, building.origin.y,
                      building.size, building.size),
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
        DrawRectangleLinesEx(screenHit, 1.0f, kHitBoxColor);

        const Vector2 ground = GetWorldToScreen2D(unit_layout::groundPoint(unit.x, unit.y), cam);
        DrawCircleV(ground, 2.0f, kPositionColor);
        drawLabel(TextFormat("#%u %s (%.2f, %.2f)", static_cast<unsigned>(entity),
                      model::unitSpec(unit.type).label.data(), static_cast<double>(unit.x),
                      static_cast<double>(unit.y)),
            screenHit.x + screenHit.width / 2.0f, screenHit.y);
    }
}

} // namespace view
