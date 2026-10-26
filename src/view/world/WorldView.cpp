#include "view/world/WorldView.hpp"

#include "view/build/BuildingRenderer.hpp"
#include "view/IsoProjection.hpp"
#include "view/IsoShapes.hpp"
#include "view/ui/Shapes.hpp"

#include <array>
#include <climits>
#include <cmath>
#include <cstddef>

namespace view::gameplay {

namespace {

constexpr Color kGroundColor{58, 94, 52, 255};
constexpr Color kOutsideMapColor{24, 48, 70, 255};
constexpr Color kValidColor{80, 220, 100, 110};
constexpr Color kInvalidColor{230, 60, 50, 130};
constexpr Color kSelectedFill{90, 255, 120, 50};
constexpr Color kSelectedOutline{90, 255, 120, 255};
constexpr unsigned char kGhostAlpha = 150;
constexpr float kFixedZoom = 1.0f;

} // namespace

WorldView::WorldView(core::EventBus& bus)
    : bus_(bus)
    , objects_(bus)
{
    bus_.subscribe<events::WorldReset>([this](const events::WorldReset&) {
        preview_ = {};
        selectionBox_ = {};
        worldMap_.reset();
        resetCamera();
    });
    bus_.subscribe<events::MapGenerated>([this](const events::MapGenerated& event) { worldMap_ = event; });
    bus_.subscribe<events::LocalPlayerChanged>([this](const events::LocalPlayerChanged& e) {
        localPlayerId_ = e.player;
        if (const auto position = objects_.playerGridPosition(localPlayerId_)) {
            focusCameraOn(position->x, position->y);
        }
    });
    bus_.subscribe<events::PlayerSpawned>([this](const events::PlayerSpawned& event) {
        if (event.player == localPlayerId_) {
            focusCameraOn(event.x, event.y);
        }
    });
    bus_.subscribe<events::PlayerMoved>([this](const events::PlayerMoved& event) { onPlayerMoved(event); });
    bus_.subscribe<events::BuildPreviewChanged>([this](const events::BuildPreviewChanged& e) {
        if (e.player == localPlayerId_) {
            preview_ = e;
        }
    });
    bus_.subscribe<events::SelectionBoxChanged>([this](const events::SelectionBoxChanged& e) { selectionBox_ = e; });
}

void WorldView::draw()
{
    ui::fill(Rectangle{0.0F, 0.0F, static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())},
        worldMap_ ? kOutsideMapColor : kGroundColor);

    const Camera2D cam = camera();
    BeginMode2D(cam);
    drawGrid(cam);
    drawDeposits();
    objects_.draw();
    drawPreview();
    EndMode2D();
    drawSelectionBox();
}

void WorldView::onPlayerMoved(const events::PlayerMoved& event)
{
    if (event.player == localPlayerId_) {
        focusCameraOn(event.x, event.y);
    }
}

void WorldView::focusCameraOn(float x, float y)
{
    cameraTarget_ = iso::gridToWorld(x, y);
    publishCamera();
}

void WorldView::resetCamera()
{
    cameraTarget_ = Vector2{0.0f, 0.0f};
    cameraZoom_ = kFixedZoom;
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

    if (worldMap_) {
        const int mapMaxX = worldMap_->origin.x + worldMap_->width - 1;
        const int mapMaxY = worldMap_->origin.y + worldMap_->height - 1;
        minX = std::max(minX, worldMap_->origin.x);
        minY = std::max(minY, worldMap_->origin.y);
        maxX = std::min(maxX, mapMaxX);
        maxY = std::min(maxY, mapMaxY);
        if (minX > maxX || minY > maxY) {
            return;
        }

        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                const int localX = x - worldMap_->origin.x;
                const int localY = y - worldMap_->origin.y;
                const std::size_t index
                    = static_cast<std::size_t>(localY) * static_cast<std::size_t>(worldMap_->width)
                    + static_cast<std::size_t>(localX);
                environment_.drawTile(worldMap_->terrain[index], static_cast<float>(x), static_cast<float>(y));
            }
        }
    }

}

void WorldView::drawDeposits() const
{
    if (!worldMap_) {
        return;
    }
    for (const model::ResourceDeposit& deposit : worldMap_->deposits) {
        resourceDeposits_.draw(deposit);
    }
}

void WorldView::drawSelectionBox() const
{
    if (!selectionBox_.visible) {
        return;
    }
    const Rectangle box{std::min(selectionBox_.x0, selectionBox_.x1), std::min(selectionBox_.y0, selectionBox_.y1),
        std::abs(selectionBox_.x1 - selectionBox_.x0), std::abs(selectionBox_.y1 - selectionBox_.y0)};
    ui::fill(box, kSelectedFill);
    ui::outline(box, kSelectedOutline);
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
    objects_.drawBuildingPreview(preview_.type, preview_.origin, preview_.size, kGhostAlpha);
}

} // namespace view::gameplay
