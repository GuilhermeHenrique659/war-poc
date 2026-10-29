#include "controller/PlayingInputController.hpp"

#include "core/Events.hpp"
#include "model/player/Player.hpp"
#include "view/build/BuildBarLayout.hpp"
#include "view/economy/InventoryPanelLayout.hpp"
#include "view/economy/LogisticsPanelLayout.hpp"
#include "view/economy/TradePanelLayout.hpp"
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
        panelEntries_.clear();
        if (!e.open) {
            return;
        }
        panelEntries_ = e.entries;
    });
    bus_.subscribe<events::InventoryPanelChanged>([this](const events::InventoryPanelChanged& e) {
        if (e.player == localPlayerId_) {
            inventoryPanel_ = e;
        }
    });
    bus_.subscribe<events::LogisticsPanelChanged>([this](const events::LogisticsPanelChanged& e) {
        if (e.player == localPlayerId_) {
            logisticsPanel_ = e;
        }
    });
    bus_.subscribe<events::TradePanelChanged>([this](const events::TradePanelChanged& e) {
        if (e.player == localPlayerId_) {
            tradePanel_ = e;
        }
    });
    bus_.subscribe<events::WorldReset>([this](const events::WorldReset&) {
        lastTile_.reset();
        placing_ = false;
        panelEntries_.clear();
        inventoryPanel_ = {};
        logisticsPanel_ = {};
        tradePanel_ = {};
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
    // Published every frame, including the zero direction, so the model knows when the avatar
    // stopped moving and can publish the idle state.
    bus_.publish(events::PlayerMoveRequested{localPlayerId_, directionX, directionY, GetFrameTime()});
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

    if (logisticsPanel_.open) {
        const int screenWidth = GetScreenWidth();
        const Rectangle bounds = view::logistics_panel_layout::panelBounds(
            logisticsPanel_.routes.size(), logisticsPanel_.warehouse, screenWidth);
        if (CheckCollisionPointRec(mouse, bounds)) {
            if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                return;
            }
            const Rectangle close = view::logistics_panel_layout::closeBounds(
                logisticsPanel_.routes.size(), logisticsPanel_.warehouse, screenWidth);
            if (CheckCollisionPointRec(mouse, close)) {
                bus_.publish(events::SelectionClearRequested{localPlayerId_});
                return;
            }
            if (logisticsPanel_.warehouse) {
                const Rectangle recruit = view::logistics_panel_layout::recruitBounds(
                    logisticsPanel_.routes.size(), screenWidth);
                if (CheckCollisionPointRec(mouse, recruit)) {
                    bus_.publish(events::RecruitDroneRequested{localPlayerId_, logisticsPanel_.building});
                    return;
                }
            }
            for (std::size_t index = 0; index < logisticsPanel_.routes.size(); ++index) {
                const auto& route = logisticsPanel_.routes[index];
                const Rectangle unassign = view::logistics_panel_layout::unassignBounds(
                    index, logisticsPanel_.routes.size(), screenWidth);
                if (CheckCollisionPointRec(mouse, unassign) && route.assignedDrones > 0) {
                    bus_.publish(events::UnassignDroneFromRouteRequested{
                        localPlayerId_, logisticsPanel_.building, route.id});
                    return;
                }
                const Rectangle assign = view::logistics_panel_layout::assignBounds(
                    index, logisticsPanel_.routes.size(), screenWidth);
                if (CheckCollisionPointRec(mouse, assign) && logisticsPanel_.idleDrones > 0) {
                    bus_.publish(events::AssignDroneToRouteRequested{
                        localPlayerId_, logisticsPanel_.building, route.id});
                    return;
                }
            }
            return;
        }
    }

    if (tradePanel_.open) {
        const int screenWidth = GetScreenWidth();
        const Rectangle tradeBounds = view::trade_panel_layout::panelBounds(tradePanel_.rows.size(), screenWidth);
        if (CheckCollisionPointRec(mouse, tradeBounds)) {
            if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                return;
            }
            const Rectangle close
                = view::trade_panel_layout::closeBounds(tradePanel_.rows.size(), screenWidth);
            if (CheckCollisionPointRec(mouse, close)) {
                bus_.publish(events::SelectionClearRequested{localPlayerId_});
            }
            return;
        }
    }

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

        const auto* playerPane = events::findPane(inventoryPanel_, events::InventoryPaneRole::Player);
        const auto* targetPane
            = events::findPane(inventoryPanel_, events::InventoryPaneRole::TransferTarget);
        const std::size_t playerPaneIndex
            = events::findPaneIndex(inventoryPanel_, events::InventoryPaneRole::Player);
        const std::size_t targetPaneIndex
            = events::findPaneIndex(inventoryPanel_, events::InventoryPaneRole::TransferTarget);

        if (!inventoryDragSlot_ && playerPane != nullptr && targetPane != nullptr
            && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            const auto startDrag = [this, mouse, screenWidth](const events::InventoryPaneState& pane,
                                      std::size_t paneIndex, bool fromPlayer) {
                for (std::size_t index = 0; index < pane.slots.size(); ++index) {
                    const Rectangle bounds
                        = view::inventory_panel_layout::slotBounds(index, paneIndex, screenWidth);
                    if (!pane.slots[index].occupied || !CheckCollisionPointRec(mouse, bounds)) {
                        continue;
                    }
                    inventoryDragSlot_ = index;
                    inventoryDragFromPlayer_ = fromPlayer;
                    bus_.publish(events::InventoryDragChanged{localPlayerId_, true,
                        pane.slots[index].item, pane.slots[index].currentQuantity});
                    return true;
                }
                return false;
            };
            if (startDrag(*playerPane, playerPaneIndex, true)
                || startDrag(*targetPane, targetPaneIndex, false)) {
                return;
            }
        }

        if (inventoryDragSlot_ && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            const std::size_t dropPaneIndex = inventoryDragFromPlayer_ ? targetPaneIndex : playerPaneIndex;
            const bool overDropPane = CheckCollisionPointRec(
                mouse, view::inventory_panel_layout::paneBounds(dropPaneIndex, screenWidth));
            if (targetPane != nullptr && overDropPane) {
                bus_.publish(events::TransferInventoryItemRequested{
                    localPlayerId_, targetPane->entity, *inventoryDragSlot_, inventoryDragFromPlayer_});
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
    if (CheckCollisionPointRec(mouse, view::recruit_panel_layout::closeBounds(panelEntries_.size(), screenWidth))) {
        bus_.publish(events::SelectionClearRequested{localPlayerId_});
        return;
    }
    for (std::size_t i = 0; i < panelEntries_.size(); ++i) {
        const Rectangle slot = view::recruit_panel_layout::slotBounds(i, panelEntries_.size(), screenWidth);
        if (!CheckCollisionPointRec(mouse, slot)) {
            continue;
        }
        const events::RecruitEntry& entry = panelEntries_[i];
        if (entry.kind == events::RecruitEntryKind::ConstructionDrone) {
            bus_.publish(events::RecruitConstructionDroneRequested{localPlayerId_});
        } else {
            bus_.publish(events::RecruitRequested{localPlayerId_, entry.type});
        }
        return;
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
    if (logisticsPanel_.open
        && CheckCollisionPointRec(mouse, view::logistics_panel_layout::panelBounds(
            logisticsPanel_.routes.size(), logisticsPanel_.warehouse, GetScreenWidth()))) {
        return true;
    }
    if (inventoryPanel_.open
        && CheckCollisionPointRec(mouse, view::inventory_panel_layout::panelBounds(GetScreenWidth()))) {
        return true;
    }
    if (panelEntries_.empty()) {
        return false;
    }
    return CheckCollisionPointRec(mouse, view::recruit_panel_layout::panelBounds(panelEntries_.size(), GetScreenWidth()));
}

} // namespace controller
