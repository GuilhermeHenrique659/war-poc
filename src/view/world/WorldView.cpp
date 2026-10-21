#include "view/world/WorldView.hpp"

#include "model/player/Player.hpp"
#include "view/build/BuildingRenderer.hpp"
#include "view/IsoProjection.hpp"
#include "view/IsoShapes.hpp"
#include "view/player/PlayerRenderer.hpp"
#include "view/units/UnitRenderer.hpp"

#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <cstddef>

namespace view::gameplay {

namespace {

constexpr Color kGroundColor{58, 94, 52, 255};
constexpr Color kOutsideMapColor{24, 48, 70, 255};
constexpr Color kWaterColor{42, 105, 156, 255};
constexpr Color kGridColor{74, 116, 66, 255};
constexpr Color kOriginColor{96, 140, 86, 255};
constexpr Color kValidColor{80, 220, 100, 110};
constexpr Color kInvalidColor{230, 60, 50, 130};
constexpr Color kSelectedFill{90, 255, 120, 50};
constexpr Color kSelectedOutline{90, 255, 120, 255};
constexpr Color kDroneColor{70, 220, 230, 255};
constexpr unsigned char kGhostAlpha = 150;
constexpr float kFixedZoom = 1.0f;

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
        drones_.clear();
        players_.clear();
        selectedUnits_.clear();
        selectedBuilding_.reset();
        preview_ = {};
        selectionBox_ = {};
        worldMap_.reset();
        resetCamera();
    });
    bus_.subscribe<events::MapGenerated>([this](const events::MapGenerated& event) { worldMap_ = event; });
    bus_.subscribe<events::BuildingPlaced>([this](const events::BuildingPlaced& e) { onBuildingPlaced(e); });
    bus_.subscribe<events::BuildingRemoved>([this](const events::BuildingRemoved& e) {
        std::erase_if(buildings_, [&e](const BuildingVisual& building) { return building.entity == e.entity; });
        if (selectedBuilding_ == e.entity) selectedBuilding_.reset();
    });
    bus_.subscribe<events::LocalPlayerChanged>([this](const events::LocalPlayerChanged& e) {
        localPlayerId_ = e.player;
        for (const auto& [entity, player] : players_) {
            (void)entity;
            if (player.player == localPlayerId_) {
                focusCameraOn(player.x, player.y);
                break;
            }
        }
    });
    bus_.subscribe<events::BuildPreviewChanged>([this](const events::BuildPreviewChanged& e) {
        if (e.player == localPlayerId_) {
            preview_ = e;
        }
    });
    bus_.subscribe<events::CameraPanRequested>([this](const events::CameraPanRequested& e) { onPan(e); });
    subscribeUnits();
    subscribePlayers();
}

void WorldView::subscribeUnits()
{
    bus_.subscribe<events::UnitSpawned>([this](const events::UnitSpawned& e) {
        units_[e.entity] = UnitVisual{e.entity, e.type, e.x, e.y, e.health, e.maxHealth, e.size};
    });
    bus_.subscribe<events::UnitRemoved>([this](const events::UnitRemoved& e) {
        units_.erase(e.entity);
        drones_.erase(e.entity);
        selectedUnits_.erase(e.entity);
    });
    bus_.subscribe<events::DroneSpawned>([this](const events::DroneSpawned& e) {
        drones_[e.entity] = DroneVisual{e.entity, e.x, e.y};
    });
    bus_.subscribe<events::DroneMoved>([this](const events::DroneMoved& e) {
        const auto it = drones_.find(e.entity);
        if (it != drones_.end()) {
            it->second.x = e.x;
            it->second.y = e.y;
        }
    });
    bus_.subscribe<events::UnitMoved>([this](const events::UnitMoved& e) {
        const auto it = units_.find(e.entity);
        if (it == units_.end()) {
            return;
        }
        it->second.x = e.x;
        it->second.y = e.y;
    });
    bus_.subscribe<events::HealthChanged>([this](const events::HealthChanged& e) {
        const auto it = units_.find(e.entity);
        if (it != units_.end()) {
            it->second.health = e.health;
            it->second.maxHealth = e.maxHealth;
        }
    });
    bus_.subscribe<events::SelectionChanged>([this](const events::SelectionChanged& e) { onSelectionChanged(e); });
    bus_.subscribe<events::SelectionBoxChanged>([this](const events::SelectionBoxChanged& e) { selectionBox_ = e; });
}

void WorldView::subscribePlayers()
{
    bus_.subscribe<events::PlayerSpawned>([this](const events::PlayerSpawned& e) {
        players_[e.entity] = PlayerVisual{e.entity, e.player, e.x, e.y};
        if (e.player == localPlayerId_) {
            focusCameraOn(e.x, e.y);
        }
    });
    bus_.subscribe<events::PlayerMoved>([this](const events::PlayerMoved& e) { onPlayerMoved(e); });
}

void WorldView::draw() const
{
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), worldMap_ ? kOutsideMapColor : kGroundColor);

    const Camera2D cam = camera();
    BeginMode2D(cam);
    drawGrid(cam);
    drawDeposits(cam);
    drawObjects();
    drawPreview();
    EndMode2D();
    drawSelectionBox();
}

void WorldView::onSelectionChanged(const events::SelectionChanged& event)
{
    if (event.player != localPlayerId_) {
        return;
    }
    selectedUnits_ = std::unordered_set<model::ecs::Entity>(event.units.begin(), event.units.end());
    selectedBuilding_ = event.building;
}

void WorldView::onPlayerMoved(const events::PlayerMoved& event)
{
    const auto player = players_.find(event.entity);
    if (player == players_.end()) {
        return;
    }

    player->second.x = event.x;
    player->second.y = event.y;
    if (event.player == localPlayerId_) {
        focusCameraOn(event.x, event.y);
    }
}

void WorldView::onBuildingPlaced(const events::BuildingPlaced& event)
{
    buildings_.push_back(BuildingVisual{event.entity, event.type, event.origin, event.size});
}

void WorldView::onPan(const events::CameraPanRequested& event)
{
    cameraTarget_.x += event.dx / cameraZoom_;
    cameraTarget_.y += event.dy / cameraZoom_;
    publishCamera();
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
                const model::Terrain terrain = worldMap_->terrain[index];
                const Color fill = terrain == model::Terrain::Water ? kWaterColor : kGroundColor;
                iso::drawFootprint(static_cast<float>(x), static_cast<float>(y), 1.0f, 1.0f, fill,
                    Color{fill.r, fill.g, fill.b, 0});
            }
        }
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

void WorldView::drawDeposits(const Camera2D& cam) const
{
    if (!worldMap_) {
        return;
    }
    for (const model::ResourceDeposit& deposit : worldMap_->deposits) {
        const Vector2 center = GetWorldToScreen2D(
            iso::gridToWorld(static_cast<float>(deposit.origin.x) + static_cast<float>(deposit.size) / 2.0F,
                static_cast<float>(deposit.origin.y) + static_cast<float>(deposit.size) / 2.0F),
            cam);
        constexpr float kHorizontalMargin = 64.0F;
        constexpr float kVerticalMargin = 32.0F;
        if (center.x < -kHorizontalMargin || center.x > static_cast<float>(GetScreenWidth()) + kHorizontalMargin
            || center.y < -kVerticalMargin || center.y > static_cast<float>(GetScreenHeight()) + kVerticalMargin) {
            continue;
        }
        Color color{};
        switch (deposit.resource) {
        case model::ItemId::Iron: color = Color{170, 180, 190, 190}; break;
        case model::ItemId::Copper: color = Color{202, 119, 65, 190}; break;
        case model::ItemId::Coal: color = Color{55, 60, 68, 210}; break;
        case model::ItemId::Steel:
        case model::ItemId::Electronics:
        case model::ItemId::Tools:
        case model::ItemId::Count: continue;
        }
        Color outline = color;
        outline.a = 255;
        iso::drawFootprint(static_cast<float>(deposit.origin.x), static_cast<float>(deposit.origin.y),
            static_cast<float>(deposit.size), static_cast<float>(deposit.size), color, outline);
    }
}

void WorldView::drawObjects() const
{
    enum class Kind { Building, Unit, Drone, Player };
    struct Drawable {
        float depth;
        model::ecs::Entity entity;
        Kind kind;
        const BuildingVisual* building;
        const UnitVisual* unit;
        const DroneVisual* drone;
        const PlayerVisual* player;
    };

    std::vector<Drawable> objects;
    objects.reserve(buildings_.size() + units_.size() + drones_.size() + players_.size());
    for (const BuildingVisual& building : buildings_) {
        objects.push_back(Drawable{static_cast<float>(depthKey(building.origin, building.size)),
            building.entity, Kind::Building, &building, nullptr, nullptr, nullptr});
    }
    for (const auto& [entity, unit] : units_) {
        objects.push_back(Drawable{unit.x + unit.y, entity, Kind::Unit, nullptr, &unit, nullptr, nullptr});
    }
    for (const auto& [entity, drone] : drones_) {
        objects.push_back(Drawable{drone.x + drone.y, entity, Kind::Drone, nullptr, nullptr, &drone, nullptr});
    }
    for (const auto& [entity, player] : players_) {
        objects.push_back(Drawable{player.x + player.y, entity, Kind::Player, nullptr, nullptr, nullptr, &player});
    }
    std::sort(objects.begin(), objects.end(),
        [](const Drawable& a, const Drawable& b) {
            return a.depth != b.depth ? a.depth < b.depth : a.entity < b.entity;
        });

    for (const Drawable& object : objects) {
        switch (object.kind) {
        case Kind::Building:
            if (selectedBuilding_ == object.entity) {
                constexpr float kMargin = 0.2f;
                const BuildingVisual& building = *object.building;
                const float x = static_cast<float>(building.origin.x) - kMargin;
                const float y = static_cast<float>(building.origin.y) - kMargin;
                const float size = static_cast<float>(building.size) + 2.0f * kMargin;
                iso::drawFootprint(x, y, size, size, kSelectedFill, kSelectedOutline);
            }
            drawBuilding(object.building->type, object.building->origin, object.building->size);
            break;
        case Kind::Unit:
            if (selectedUnits_.contains(object.entity)) {
                drawUnitSelectionRing(object.unit->x, object.unit->y, object.unit->size);
            }
            drawUnit(object.unit->type, object.unit->x, object.unit->y, object.unit->size);
            if (object.unit->maxHealth > 0) {
                drawUnitHealthBar(object.unit->x, object.unit->y, object.unit->size,
                    static_cast<float>(object.unit->health) / static_cast<float>(object.unit->maxHealth));
            }
            break;
        case Kind::Drone: {
            const DroneVisual& drone = *object.drone;
            const Vector2 ground = iso::gridToWorld(drone.x, drone.y);
            DrawEllipse(static_cast<int>(ground.x), static_cast<int>(ground.y), 9.0F, 5.0F, Color{0, 0, 0, 70});
            iso::drawPrism(iso::Prism{drone.x - 0.18F, drone.y - 0.18F, 0.36F, 0.36F, 7.0F, 7.0F},
                iso::boxColors(kDroneColor, 255));
            DrawLineEx(Vector2{ground.x - 7.0F, ground.y - 2.0F},
                Vector2{ground.x + 7.0F, ground.y - 2.0F}, 2.0F, RAYWHITE);
            break;
        }
        case Kind::Player:
            drawPlayerAvatar(object.player->x, object.player->y);
            break;
        }
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

} // namespace view::gameplay
