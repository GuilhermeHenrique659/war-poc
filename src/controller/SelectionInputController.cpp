#include "controller/SelectionInputController.hpp"

#include "core/Events.hpp"
#include "view/IsoProjection.hpp"
#include "view/UnitLayout.hpp"

#include <algorithm>
#include <cmath>

namespace controller {

namespace {

constexpr float kDragThreshold = 5.0f; // Screen pixels before a press becomes a box.

[[nodiscard]] bool shiftDown()
{
    return IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
}

} // namespace

SelectionInputController::SelectionInputController(core::EventBus& bus)
    : bus_(bus)
{
    subscribe();
}

void SelectionInputController::subscribe()
{
    bus_.subscribe<events::LocalPlayerChanged>([this](const events::LocalPlayerChanged& e) { localPlayerId_ = e.player; });
    bus_.subscribe<events::WorldReset>([this](const events::WorldReset&) {
        units_.clear();
        buildings_.clear();
        hasUnitSelection_ = false;
        hasBuildingSelection_ = false;
        cancelDrag();
    });
    bus_.subscribe<events::UnitSpawned>([this](const events::UnitSpawned& e) {
        units_[e.entity] = UnitInfo{e.owner, e.x, e.y, e.size};
    });
    bus_.subscribe<events::UnitRemoved>([this](const events::UnitRemoved& e) { units_.erase(e.entity); });
    bus_.subscribe<events::UnitMoved>([this](const events::UnitMoved& e) {
        const auto it = units_.find(e.entity);
        if (it == units_.end()) {
            return;
        }
        it->second.x = e.x;
        it->second.y = e.y;
    });
    bus_.subscribe<events::BuildingPlaced>([this](const events::BuildingPlaced& e) {
        buildings_.push_back(BuildingInfo{e.entity, e.owner, e.origin, e.size});
    });
    bus_.subscribe<events::SelectionChanged>([this](const events::SelectionChanged& e) {
        if (e.player != localPlayerId_) {
            return;
        }
        hasUnitSelection_ = !e.units.empty();
        hasBuildingSelection_ = e.building.has_value();
    });
}

void SelectionInputController::update(Vector2 mouse, const Camera2D& camera, bool overUi)
{
    if (!overUi && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        issueMove(mouse, camera);
    }
    if (!overUi && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        dragging_ = true;
        boxActive_ = false;
        dragStart_ = mouse;
    }
    if (!dragging_) {
        return;
    }

    if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        finishDrag(mouse, camera);
        return;
    }
    if (!boxActive_ && std::hypot(mouse.x - dragStart_.x, mouse.y - dragStart_.y) >= kDragThreshold) {
        boxActive_ = true;
    }
    if (boxActive_) {
        publishBox(mouse);
    }
}

void SelectionInputController::cancelDrag()
{
    if (boxActive_) {
        bus_.publish(events::SelectionBoxChanged{false, 0.0f, 0.0f, 0.0f, 0.0f});
    }
    dragging_ = false;
    boxActive_ = false;
}

void SelectionInputController::finishDrag(Vector2 mouse, const Camera2D& camera)
{
    const bool wasBox = boxActive_;
    cancelDrag();
    if (wasBox) {
        selectInBox(mouse, camera, shiftDown());
        return;
    }
    selectAt(mouse, camera, shiftDown());
}

void SelectionInputController::selectInBox(Vector2 corner, const Camera2D& camera, bool additive)
{
    const Rectangle box{std::min(dragStart_.x, corner.x), std::min(dragStart_.y, corner.y),
        std::abs(corner.x - dragStart_.x), std::abs(corner.y - dragStart_.y)};

    events::SelectUnitsRequested request{localPlayerId_, {}, additive};
    for (const auto& [entity, unit] : units_) {
        if (unit.owner != localPlayerId_) {
            continue;
        }
        const Vector2 screen = GetWorldToScreen2D(view::unit_layout::groundPoint(unit.x, unit.y), camera);
        if (CheckCollisionPointRec(screen, box)) {
            request.units.push_back(entity);
        }
    }
    bus_.publish(request);
}

void SelectionInputController::selectAt(Vector2 mouse, const Camera2D& camera, bool additive)
{
    const Vector2 world = GetScreenToWorld2D(mouse, camera);
    if (const auto unit = unitAt(world)) {
        bus_.publish(events::SelectUnitsRequested{localPlayerId_, {*unit}, additive});
        return;
    }
    if (const auto building = buildingAt(view::iso::worldToGrid(world))) {
        bus_.publish(events::SelectBuildingRequested{localPlayerId_, *building});
        return;
    }
    if (!additive) {
        bus_.publish(events::SelectionClearRequested{localPlayerId_});
    }
}

void SelectionInputController::issueMove(Vector2 mouse, const Camera2D& camera)
{
    if (!hasUnitSelection_) {
        return;
    }
    bus_.publish(events::MoveOrderRequested{localPlayerId_, view::iso::screenToGrid(mouse, camera)});
}

void SelectionInputController::publishBox(Vector2 corner) const
{
    bus_.publish(events::SelectionBoxChanged{true, dragStart_.x, dragStart_.y, corner.x, corner.y});
}

std::optional<model::ecs::Entity> SelectionInputController::unitAt(Vector2 world) const
{
    // Several sprites may overlap: pick the one drawn in front (largest depth).
    std::optional<model::ecs::Entity> best;
    float bestDepth = 0.0f;
    for (const auto& [entity, unit] : units_) {
        if (unit.owner != localPlayerId_) {
            continue;
        }
        if (!CheckCollisionPointRec(world, view::unit_layout::hitBounds(unit.x, unit.y, unit.size))) {
            continue;
        }
        const float depth = unit.x + unit.y;
        if (!best || depth > bestDepth) {
            best = entity;
            bestDepth = depth;
        }
    }
    return best;
}

std::optional<model::ecs::Entity> SelectionInputController::buildingAt(model::GridPos tile) const
{
    for (const BuildingInfo& building : buildings_) {
        if (building.owner != localPlayerId_) {
            continue;
        }
        const bool insideX = tile.x >= building.origin.x && tile.x < building.origin.x + building.size;
        const bool insideY = tile.y >= building.origin.y && tile.y < building.origin.y + building.size;
        if (insideX && insideY) {
            return building.entity;
        }
    }
    return std::nullopt;
}

} // namespace controller
