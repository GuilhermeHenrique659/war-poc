#include "view/WorldView.hpp"

#include "model/Player.hpp"
#include "view/BuildingRenderer.hpp"
#include "view/IsoProjection.hpp"
#include "view/IsoShapes.hpp"
#include "view/UnitRenderer.hpp"

#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <cstddef>
#include <limits>

namespace view {

namespace {

constexpr Color kGroundColor{58, 94, 52, 255};
constexpr Color kGridColor{74, 116, 66, 255};
constexpr Color kOriginColor{96, 140, 86, 255};
constexpr Color kValidColor{80, 220, 100, 110};
constexpr Color kInvalidColor{230, 60, 50, 130};
constexpr Color kSelectedFill{90, 255, 120, 50};
constexpr Color kSelectedOutline{90, 255, 120, 255};
constexpr unsigned char kGhostAlpha = 150;
constexpr float kMinZoom = 0.25f;
constexpr float kMaxZoom = 2.5f;
constexpr float kZoomStep = 1.1f;

// Painter's order for square footprints: farther (smaller x + y) first.
[[nodiscard]] int depthKey(model::GridPos origin, int size)
{
    return origin.x + origin.y + size;
}

} // namespace

WorldView::WorldView(core::EventBus& bus)
    : bus_(bus)
{
    bus_.subscribe<events::WorldReset>([this](const events::WorldReset&) {
        buildings_.clear();
        units_.clear();
        selectedUnits_.clear();
        selectedBuilding_.reset();
        preview_ = {};
        selectionBox_ = {};
        resetCamera();
    });
    bus_.subscribe<events::BuildingPlaced>([this](const events::BuildingPlaced& e) { onBuildingPlaced(e); });
    bus_.subscribe<events::BuildPreviewChanged>([this](const events::BuildPreviewChanged& e) {
        if (e.player == model::kLocalPlayer) {
            preview_ = e;
        }
    });
    bus_.subscribe<events::CameraPanRequested>([this](const events::CameraPanRequested& e) { onPan(e); });
    bus_.subscribe<events::CameraZoomRequested>([this](const events::CameraZoomRequested& e) { onZoom(e); });
    subscribeUnits();
}

void WorldView::subscribeUnits()
{
    bus_.subscribe<events::UnitSpawned>([this](const events::UnitSpawned& e) {
        units_[e.entity] = UnitVisual{e.entity, e.type, e.x, e.y, e.health, e.maxHealth};
    });
    bus_.subscribe<events::UnitMoved>([this](const events::UnitMoved& e) {
        const auto it = units_.find(e.entity);
        if (it == units_.end()) {
            return;
        }
        it->second.x = e.x;
        it->second.y = e.y;
    });
    bus_.subscribe<events::SelectionChanged>([this](const events::SelectionChanged& e) { onSelectionChanged(e); });
    bus_.subscribe<events::SelectionBoxChanged>([this](const events::SelectionBoxChanged& e) { selectionBox_ = e; });
}

void WorldView::draw() const
{
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), kGroundColor);

    const Camera2D cam = camera();
    BeginMode2D(cam);
    drawGrid(cam);
    drawSelectedBuilding();
    drawObjects();
    drawHealthBars();
    drawPreview();
    EndMode2D();
    drawSelectionBox();
}

void WorldView::onSelectionChanged(const events::SelectionChanged& event)
{
    if (event.player != model::kLocalPlayer) {
        return;
    }
    selectedUnits_ = std::unordered_set<model::ecs::Entity>(event.units.begin(), event.units.end());
    selectedBuilding_ = event.building;
}

void WorldView::onBuildingPlaced(const events::BuildingPlaced& event)
{
    const BuildingVisual visual{event.entity, event.type, event.origin, event.size};
    const int key = depthKey(visual.origin, visual.size);
    const auto it = std::upper_bound(buildings_.begin(), buildings_.end(), key,
        [](int value, const BuildingVisual& other) { return value < depthKey(other.origin, other.size); });
    buildings_.insert(it, visual);
}

void WorldView::onPan(const events::CameraPanRequested& event)
{
    cameraTarget_.x += event.dx / cameraZoom_;
    cameraTarget_.y += event.dy / cameraZoom_;
    publishCamera();
}

void WorldView::onZoom(const events::CameraZoomRequested& event)
{
    // Keep the world point under the pivot fixed while zooming.
    const Vector2 pivot{event.screenX, event.screenY};
    const Camera2D before = camera();
    const Vector2 anchor = GetScreenToWorld2D(pivot, before);

    cameraZoom_ = std::clamp(cameraZoom_ * std::pow(kZoomStep, event.steps), kMinZoom, kMaxZoom);
    cameraTarget_.x = anchor.x - (pivot.x - before.offset.x) / cameraZoom_;
    cameraTarget_.y = anchor.y - (pivot.y - before.offset.y) / cameraZoom_;
    publishCamera();
}

void WorldView::resetCamera()
{
    cameraTarget_ = Vector2{0.0f, 0.0f};
    cameraZoom_ = 1.0f;
    publishCamera();
}

void WorldView::publishCamera() const
{
    bus_.publish(events::CameraChanged{cameraTarget_.x, cameraTarget_.y, cameraZoom_});
}

Camera2D WorldView::camera() const
{
    return iso::makeCamera(cameraTarget_, cameraZoom_, GetScreenWidth(), GetScreenHeight());
}

void WorldView::drawGrid(const Camera2D& cam) const
{
    const float width = static_cast<float>(GetScreenWidth());
    const float height = static_cast<float>(GetScreenHeight());
    const std::array corners{Vector2{0.0f, 0.0f}, Vector2{width, 0.0f}, Vector2{0.0f, height}, Vector2{width, height}};

    int minX = INT_MAX;
    int minY = INT_MAX;
    int maxX = INT_MIN;
    int maxY = INT_MIN;
    for (const Vector2 corner : corners) {
        const model::GridPos tile = iso::screenToGrid(corner, cam);
        minX = std::min(minX, tile.x);
        minY = std::min(minY, tile.y);
        maxX = std::max(maxX, tile.x);
        maxY = std::max(maxY, tile.y);
    }

    const float top = static_cast<float>(minY);
    const float bottom = static_cast<float>(maxY + 1);
    for (int gx = minX; gx <= maxX + 1; ++gx) {
        const float x = static_cast<float>(gx);
        DrawLineV(iso::gridToWorld(x, top), iso::gridToWorld(x, bottom), gx == 0 ? kOriginColor : kGridColor);
    }

    const float left = static_cast<float>(minX);
    const float right = static_cast<float>(maxX + 1);
    for (int gy = minY; gy <= maxY + 1; ++gy) {
        const float y = static_cast<float>(gy);
        DrawLineV(iso::gridToWorld(left, y), iso::gridToWorld(right, y), gy == 0 ? kOriginColor : kGridColor);
    }
}

void WorldView::drawSelectedBuilding() const
{
    if (!selectedBuilding_) {
        return;
    }
    const auto it = std::find_if(buildings_.begin(), buildings_.end(),
        [this](const BuildingVisual& building) { return building.entity == *selectedBuilding_; });
    if (it == buildings_.end()) {
        return;
    }

    // Slightly larger than the footprint so the outline shows around the building's base.
    constexpr float kMargin = 0.2f;
    const float x = static_cast<float>(it->origin.x) - kMargin;
    const float y = static_cast<float>(it->origin.y) - kMargin;
    const float size = static_cast<float>(it->size) + 2.0f * kMargin;
    iso::drawFootprint(x, y, size, size, kSelectedFill, kSelectedOutline);
}

void WorldView::drawObjects() const
{
    std::vector<const UnitVisual*> units;
    units.reserve(units_.size());
    for (const auto& [entity, unit] : units_) {
        units.push_back(&unit);
    }
    std::sort(units.begin(), units.end(),
        [](const UnitVisual* a, const UnitVisual* b) { return a->x + a->y < b->x + b->y; });

    // Both lists are sorted by depth; merge them while drawing.
    std::size_t next = 0;
    const auto drawBuildingsUpTo = [&](float depth) {
        while (next < buildings_.size()
            && static_cast<float>(depthKey(buildings_[next].origin, buildings_[next].size)) <= depth) {
            const BuildingVisual& building = buildings_[next++];
            drawBuilding(building.type, building.origin, building.size);
        }
    };

    for (const UnitVisual* unit : units) {
        drawBuildingsUpTo(unit->x + unit->y);
        if (selectedUnits_.contains(unit->entity)) {
            drawUnitSelectionRing(unit->x, unit->y);
        }
        drawUnit(unit->type, unit->x, unit->y);
    }
    drawBuildingsUpTo(std::numeric_limits<float>::max());
}

void WorldView::drawHealthBars() const
{
    for (const model::ecs::Entity entity : selectedUnits_) {
        const auto it = units_.find(entity);
        if (it == units_.end() || it->second.maxHealth <= 0) {
            continue;
        }
        const UnitVisual& unit = it->second;
        drawUnitHealthBar(unit.x, unit.y, static_cast<float>(unit.health) / static_cast<float>(unit.maxHealth));
    }
}

void WorldView::drawSelectionBox() const
{
    if (!selectionBox_.visible) {
        return;
    }
    const Rectangle box{std::min(selectionBox_.x0, selectionBox_.x1), std::min(selectionBox_.y0, selectionBox_.y1),
        std::abs(selectionBox_.x1 - selectionBox_.x0), std::abs(selectionBox_.y1 - selectionBox_.y0)};
    DrawRectangleRec(box, kSelectedFill);
    DrawRectangleLinesEx(box, 1.0f, kSelectedOutline);
}

void WorldView::drawPreview() const
{
    if (!preview_.visible) {
        return;
    }

    const float x = static_cast<float>(preview_.origin.x);
    const float y = static_cast<float>(preview_.origin.y);
    const float size = static_cast<float>(preview_.size);
    const Color fill = preview_.valid ? kValidColor : kInvalidColor;
    Color outline = fill;
    outline.a = 255;

    iso::drawFootprint(x, y, size, size, fill, outline);
    drawBuilding(preview_.type, preview_.origin, preview_.size, kGhostAlpha);
}

} // namespace view
