#include "controller/PlayingInputController.hpp"

#include "core/Events.hpp"
#include "model/player/Player.hpp"
#include "view/build/BuildBarLayout.hpp"
#include "view/economy/InventoryPanelLayout.hpp"
#include "view/IsoProjection.hpp"
#include "view/units/RecruitPanelLayout.hpp"

namespace controller {

PlayingInputController::PlayingInputController(core::EventBus& bus)
    : bus_(bus)
    , selection_(std::make_unique<SelectionInputController>(bus))
{
    bus_.subscribe<events::CameraChanged>([this](const events::CameraChanged& e) {
        cameraTarget_ = Vector2{e.targetX, e.targetY};
        cameraZoom_ = e.zoom;
    });
    bus_.subscribe<events::LocalPlayerChanged>([this](const events::LocalPlayerChanged& e) { localPlayerId_ = e.player; });
    bus_.subscribe<events::BuildBarChanged>([this](const events::BuildBarChanged& e) {
        if (e.player != localPlayerId_) {
            return;
        }
        barTypes_.clear();
        for (const auto& entry : e.entries) {
            barTypes_.push_back(entry.type);
        }
        hasBuildSelection_ = e.selected.has_value();
    });
    bus_.subscribe<events::RecruitPanelChanged>([this](const events::RecruitPanelChanged& e) {
        if (e.player != localPlayerId_) {
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
    bus_.subscribe<events::InventoryPanelChanged>([this](const events::InventoryPanelChanged& e) {
        if (e.player == localPlayerId_) {
            inventoryPanel_ = e;
        }
    });
    bus_.subscribe<events::WorldReset>([this](const events::WorldReset&) {
        lastTile_.reset();
        placing_ = false;
        panelTypes_.clear();
        inventoryPanel_ = {};
        inventoryDragSlot_.reset();
    });
}

void PlayingInputController::update()
{
    if (handleCancel()) {
        return;
    }
    if (IsKeyPressed(KEY_I)) {
        bus_.publish(events::TogglePlayerInventoryRequested{localPlayerId_});
        return;
    }
    handlePlayerMovement();
    handleShortcuts();

    // Sampled before handling clicks: placing the Command Center unselects it in the same frame.
    const bool wasBuilding = hasBuildSelection_;
    const Vector2 mouse = GetMousePosition();
    const Camera2D camera = view::iso::makeCamera(cameraTarget_, cameraZoom_, GetScreenWidth(), GetScreenHeight());
    const bool overPanel = isOverPanel(mouse) || inventoryDragSlot_.has_value();
    const bool overBar
        = CheckCollisionPointRec(
            mouse, view::build_bar_layout::barBounds(GetScreenWidth(), GetScreenHeight(), barTypes_.size()));

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

    if (escape && inventoryPanel_.open) {
        bus_.publish(events::CloseInventoryRequested{localPlayerId_});
        inventoryDragSlot_.reset();
        bus_.publish(events::InventoryDragChanged{localPlayerId_, false, model::ItemId::Count, 0});
        return true;
    }

    if (hasBuildSelection_) {
        bus_.publish(events::BuildCancelRequested{localPlayerId_});
        return true;
    }
    // Without a building selected, right click is a move order.
    if (!escape) {
        return false;
    }
    if (selection_->hasSelection()) {
        selection_->cancelDrag();
        bus_.publish(events::SelectionClearRequested{localPlayerId_});
        return true;
    }
    bus_.publish(events::ReturnToMenuRequested{});
    return true;
}

void PlayingInputController::handlePlayerMovement()
{
    float directionX = 0.0F;
    float directionY = 0.0F;
    if (IsKeyDown(KEY_W)) {
        directionX -= 1.0F;
        directionY -= 1.0F;
    }
    if (IsKeyDown(KEY_A)) {
        directionX -= 1.0F;
        directionY += 1.0F;
    }
    if (IsKeyDown(KEY_S)) {
        directionX += 1.0F;
        directionY += 1.0F;
    }
    if (IsKeyDown(KEY_D)) {
        directionX += 1.0F;
        directionY -= 1.0F;
    }
    if (directionX != 0.0F || directionY != 0.0F) {
        bus_.publish(events::PlayerMoveRequested{localPlayerId_, directionX, directionY, GetFrameTime()});
    }

}

void PlayingInputController::handleShortcuts()
{
    for (std::size_t i = 0; i < barTypes_.size() && i < view::build_bar_layout::kMaxShortcuts; ++i) {
        if (IsKeyPressed(KEY_ONE + static_cast<int>(i))) {
            bus_.publish(events::BuildSelectRequested{localPlayerId_, barTypes_[i]});
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
    bus_.publish(events::BuildSelectRequested{localPlayerId_, barTypes_[*slot]});
}

void PlayingInputController::handlePanelMouse(Vector2 mouse)
{
    placing_ = false;
    publishCursor(std::nullopt);

    if (inventoryPanel_.open) {
        const int screenWidth = GetScreenWidth();
        const Rectangle inventoryBounds = view::inventory_panel_layout::panelBounds(screenWidth);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)
            && CheckCollisionPointRec(mouse, view::inventory_panel_layout::closeBounds(screenWidth))) {
            bus_.publish(events::CloseInventoryRequested{localPlayerId_});
            inventoryDragSlot_.reset();
            bus_.publish(events::InventoryDragChanged{localPlayerId_, false, model::ItemId::Count, 0});
            return;
        }

        if (!inventoryDragSlot_ && inventoryPanel_.hasBuilding
            && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            for (std::size_t index = 0; index < inventoryPanel_.playerSlots.size(); ++index) {
                const Rectangle slot = view::inventory_panel_layout::slotBounds(index, true, screenWidth);
                if (inventoryPanel_.playerSlots[index].occupied && CheckCollisionPointRec(mouse, slot)) {
                    inventoryDragSlot_ = index;
                    inventoryDragFromPlayer_ = true;
                    bus_.publish(events::InventoryDragChanged{
                        localPlayerId_, true, inventoryPanel_.playerSlots[index].item,
                        inventoryPanel_.playerSlots[index].currentQuantity});
                    return;
                }
            }
            for (std::size_t index = 0; index < inventoryPanel_.buildingSlots.size(); ++index) {
                const Rectangle slot = view::inventory_panel_layout::slotBounds(index, false, screenWidth);
                if (inventoryPanel_.buildingSlots[index].occupied && CheckCollisionPointRec(mouse, slot)) {
                    inventoryDragSlot_ = index;
                    inventoryDragFromPlayer_ = false;
                    bus_.publish(events::InventoryDragChanged{
                        localPlayerId_, true, inventoryPanel_.buildingSlots[index].item,
                        inventoryPanel_.buildingSlots[index].currentQuantity});
                    return;
                }
            }
        }

        if (inventoryDragSlot_ && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            const bool targetIsPlayer = !inventoryDragFromPlayer_;
            const Rectangle targetPane = view::inventory_panel_layout::paneBounds(targetIsPlayer, screenWidth);
            if (CheckCollisionPointRec(mouse, targetPane)) {
                bus_.publish(events::TransferInventoryItemRequested{
                    localPlayerId_, inventoryPanel_.building, *inventoryDragSlot_, inventoryDragFromPlayer_});
            }
            inventoryDragSlot_.reset();
            bus_.publish(events::InventoryDragChanged{localPlayerId_, false, model::ItemId::Count, 0});
            return;
        }

        if (CheckCollisionPointRec(mouse, inventoryBounds)) {
            return;
        }
    }

    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        return;
    }
    const int screenWidth = GetScreenWidth();
    if (CheckCollisionPointRec(mouse, view::recruit_panel_layout::closeBounds(panelTypes_.size(), screenWidth))) {
        bus_.publish(events::SelectionClearRequested{localPlayerId_});
        return;
    }
    for (std::size_t i = 0; i < panelTypes_.size(); ++i) {
        const Rectangle slot = view::recruit_panel_layout::slotBounds(i, panelTypes_.size(), screenWidth);
        if (CheckCollisionPointRec(mouse, slot)) {
            bus_.publish(events::RecruitRequested{localPlayerId_, panelTypes_[i]});
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
        bus_.publish(events::BuildPlaceRequested{localPlayerId_, tile});
    }
}

void PlayingInputController::publishCursor(std::optional<model::GridPos> tile)
{
    if (lastTile_ == tile) {
        return;
    }
    lastTile_ = tile;
    bus_.publish(events::BuildCursorMoved{localPlayerId_, tile});
}

std::optional<std::size_t> PlayingInputController::slotUnderMouse(Vector2 mouse) const
{
    for (std::size_t i = 0; i < barTypes_.size(); ++i) {
        if (CheckCollisionPointRec(
                mouse, view::build_bar_layout::slotBounds(i, GetScreenWidth(), GetScreenHeight(), barTypes_.size()))) {
            return i;
        }
    }
    return std::nullopt;
}

bool PlayingInputController::isOverPanel(Vector2 mouse) const
{
    if (inventoryPanel_.open
        && CheckCollisionPointRec(mouse, view::inventory_panel_layout::panelBounds(GetScreenWidth()))) {
        return true;
    }
    if (panelTypes_.empty()) {
        return false;
    }
    return CheckCollisionPointRec(mouse, view::recruit_panel_layout::panelBounds(panelTypes_.size(), GetScreenWidth()));
}

} // namespace controller
