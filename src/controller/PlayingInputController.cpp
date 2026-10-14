#include "controller/PlayingInputController.hpp"

#include "core/Events.hpp"
#include "model/Player.hpp"
#include "view/BuildBarLayout.hpp"
#include "view/IsoProjection.hpp"
#include "view/MergePanelLayout.hpp"
#include "view/RecruitPanelLayout.hpp"

namespace controller {

namespace {

constexpr float kPanSpeed = 700.0f; // Screen pixels per second.
constexpr std::size_t kMaxShortcuts = 9;

} // namespace

PlayingInputController::PlayingInputController(core::EventBus& bus)
    : bus_(bus)
    , selection_(std::make_unique<SelectionInputController>(bus))
{
    bus_.subscribe<events::CameraChanged>([this](const events::CameraChanged& e) {
        cameraTarget_ = Vector2{e.targetX, e.targetY};
        cameraZoom_ = e.zoom;
    });
    bus_.subscribe<events::BuildBarChanged>([this](const events::BuildBarChanged& e) {
        if (e.player != model::kLocalPlayer) {
            return;
        }
        barTypes_.clear();
        for (const auto& entry : e.entries) {
            barTypes_.push_back(entry.type);
        }
        hasBuildSelection_ = e.selected.has_value();
    });
    bus_.subscribe<events::RecruitPanelChanged>([this](const events::RecruitPanelChanged& e) {
        if (e.player != model::kLocalPlayer) {
            return;
        }
        panelTypes_.clear();
        if (!e.open) {
            return;
        }
        for (const auto& entry : e.entries) {
            panelTypes_.push_back(entry.type);
        }
    });
    bus_.subscribe<events::MergePanelChanged>([this](const events::MergePanelChanged& e) {
        if (e.player == model::kLocalPlayer) {
            mergeOpen_ = e.open;
        }
    });
    bus_.subscribe<events::WorldReset>([this](const events::WorldReset&) {
        lastTile_.reset();
        placing_ = false;
        panelTypes_.clear();
        mergeOpen_ = false;
    });
}

void PlayingInputController::update()
{
    if (handleCancel()) {
        return;
    }
    handleCamera();
    handleShortcuts();

    // Sampled before handling clicks: placing the Command Center unselects it in the same frame.
    const bool wasBuilding = hasBuildSelection_;
    const Vector2 mouse = GetMousePosition();
    const Camera2D camera = view::iso::makeCamera(cameraTarget_, cameraZoom_, GetScreenWidth(), GetScreenHeight());
    const bool overPanel = isOverPanel(mouse);
    const bool overBar
        = CheckCollisionPointRec(mouse, view::build_bar_layout::barBounds(GetScreenWidth(), GetScreenHeight()));

    if (overPanel) {
        handlePanelMouse(mouse);
    } else if (overBar) {
        handleBarMouse(mouse);
    } else {
        handleMapMouse(mouse, camera);
    }

    // While a building is selected, map clicks place it instead of selecting.
    if (wasBuilding || hasBuildSelection_) {
        selection_->cancelDrag();
        return;
    }
    selection_->update(mouse, camera, overPanel || overBar);
}

bool PlayingInputController::handleCancel()
{
    const bool escape = IsKeyPressed(KEY_ESCAPE);
    const bool rightClick = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
    if (!escape && !rightClick) {
        return false;
    }

    if (hasBuildSelection_) {
        bus_.publish(events::BuildCancelRequested{model::kLocalPlayer});
        return true;
    }
    // Without a building selected, right click is a move order.
    if (!escape) {
        return false;
    }
    if (selection_->hasSelection()) {
        selection_->cancelDrag();
        bus_.publish(events::SelectionClearRequested{model::kLocalPlayer});
        return true;
    }
    bus_.publish(events::ReturnToMenuRequested{});
    return true;
}

void PlayingInputController::handleCamera()
{
    Vector2 direction{0.0f, 0.0f};
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) {
        direction.x -= 1.0f;
    }
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) {
        direction.x += 1.0f;
    }
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) {
        direction.y -= 1.0f;
    }
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) {
        direction.y += 1.0f;
    }
    if (direction.x != 0.0f || direction.y != 0.0f) {
        const float step = kPanSpeed * GetFrameTime();
        bus_.publish(events::CameraPanRequested{direction.x * step, direction.y * step});
    }

    if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
        const Vector2 delta = GetMouseDelta();
        if (delta.x != 0.0f || delta.y != 0.0f) {
            bus_.publish(events::CameraPanRequested{-delta.x, -delta.y});
        }
    }

    const float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        const Vector2 mouse = GetMousePosition();
        bus_.publish(events::CameraZoomRequested{wheel, mouse.x, mouse.y});
    }
}

void PlayingInputController::handleShortcuts()
{
    if (mergeOpen_ && IsKeyPressed(KEY_M)) {
        bus_.publish(events::MergeRequested{model::kLocalPlayer});
        return;
    }
    for (std::size_t i = 0; i < barTypes_.size() && i < kMaxShortcuts; ++i) {
        if (IsKeyPressed(KEY_ONE + static_cast<int>(i))) {
            bus_.publish(events::BuildSelectRequested{model::kLocalPlayer, barTypes_[i]});
            return;
        }
    }
}

void PlayingInputController::handleBarMouse(Vector2 mouse)
{
    placing_ = false;
    publishCursor(std::nullopt);

    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        return;
    }
    const auto slot = slotUnderMouse(mouse);
    if (!slot) {
        return;
    }
    bus_.publish(events::BuildSelectRequested{model::kLocalPlayer, barTypes_[*slot]});
}

void PlayingInputController::handlePanelMouse(Vector2 mouse)
{
    placing_ = false;
    publishCursor(std::nullopt);

    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        return;
    }
    const int screenWidth = GetScreenWidth();
    if (mergeOpen_) {
        if (CheckCollisionPointRec(mouse, view::merge_panel_layout::buttonBounds(screenWidth))) {
            bus_.publish(events::MergeRequested{model::kLocalPlayer});
        }
        return;
    }
    if (CheckCollisionPointRec(mouse, view::recruit_panel_layout::closeBounds(panelTypes_.size(), screenWidth))) {
        bus_.publish(events::SelectionClearRequested{model::kLocalPlayer});
        return;
    }
    for (std::size_t i = 0; i < panelTypes_.size(); ++i) {
        const Rectangle slot = view::recruit_panel_layout::slotBounds(i, panelTypes_.size(), screenWidth);
        if (CheckCollisionPointRec(mouse, slot)) {
            bus_.publish(events::RecruitRequested{model::kLocalPlayer, panelTypes_[i]});
            return;
        }
    }
}

void PlayingInputController::handleMapMouse(Vector2 mouse, const Camera2D& camera)
{
    const model::GridPos tile = view::iso::screenToGrid(mouse, camera);
    const bool tileChanged = lastTile_ != tile;
    publishCursor(tile);

    const bool pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    if (pressed) {
        placing_ = true;
    }
    if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        placing_ = false;
    }

    if (!hasBuildSelection_) {
        return;
    }
    // Dragging keeps placing on every new tile, so walls can be painted.
    if (pressed || (placing_ && tileChanged)) {
        bus_.publish(events::BuildPlaceRequested{model::kLocalPlayer, tile});
    }
}

void PlayingInputController::publishCursor(std::optional<model::GridPos> tile)
{
    if (lastTile_ == tile) {
        return;
    }
    lastTile_ = tile;
    bus_.publish(events::BuildCursorMoved{model::kLocalPlayer, tile});
}

std::optional<std::size_t> PlayingInputController::slotUnderMouse(Vector2 mouse) const
{
    for (std::size_t i = 0; i < barTypes_.size(); ++i) {
        if (CheckCollisionPointRec(mouse, view::build_bar_layout::slotBounds(i, GetScreenWidth(), GetScreenHeight()))) {
            return i;
        }
    }
    return std::nullopt;
}

bool PlayingInputController::isOverPanel(Vector2 mouse) const
{
    if (mergeOpen_ && CheckCollisionPointRec(mouse, view::merge_panel_layout::panelBounds(GetScreenWidth()))) {
        return true;
    }
    if (panelTypes_.empty()) {
        return false;
    }
    return CheckCollisionPointRec(mouse, view::recruit_panel_layout::panelBounds(panelTypes_.size(), GetScreenWidth()));
}

} // namespace controller
