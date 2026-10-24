#include "view/world/WorldObjectsViewModel.hpp"

#include "view/build/BuildingRenderer.hpp"
#include "view/IsoProjection.hpp"
#include "view/IsoShapes.hpp"
#include "view/units/UnitRenderer.hpp"

#include <algorithm>
#include <vector>

namespace view::gameplay {

namespace {

constexpr Color kSelectedFill{90, 255, 120, 50};
constexpr Color kSelectedOutline{90, 255, 120, 255};

[[nodiscard]] int depthKey(model::GridPos origin, int size)
{
    return origin.x + origin.y + size;
}

} // namespace

WorldObjectsViewModel::WorldObjectsViewModel(core::EventBus& bus)
    : bus_(bus)
    , commandCenterViewModel_{loadCommandCenterViewModel()}
    , warehouseViewModel_{loadWarehouseViewModel()}
    , steelworksViewModel_{loadSteelworksViewModel()}
    , barracksViewModel_{loadBarracksViewModel()}
    , shotEffects_(bus)
{
    bus_.subscribe<events::WorldReset>([this](const events::WorldReset&) {
        buildings_.clear();
        units_.clear();
        drones_.clear();
        players_.clear();
        selectedUnits_.clear();
        selectedBuilding_.reset();
    });
    bus_.subscribe<events::BuildingPlaced>([this](const events::BuildingPlaced& event) {
        const int health = model::buildingMaxHealth(event.type);
        buildings_.push_back(BuildingVisual{
            event.entity, event.type, event.origin, event.size, health, health,
            std::make_unique<BuildingViewModel>(event.type, event.size, mineResourceViewModel_,
                commandCenterViewModel_, warehouseViewModel_, steelworksViewModel_, factoryViewModel_,
                barracksViewModel_)
        });
    });
    bus_.subscribe<events::BuildingRemoved>([this](const events::BuildingRemoved& event) {
        std::erase_if(buildings_, [&event](const BuildingVisual& building) {
            return building.entity == event.entity;
        });
        if (selectedBuilding_ == event.entity) {
            selectedBuilding_.reset();
        }
    });
    bus_.subscribe<events::LocalPlayerChanged>([this](const events::LocalPlayerChanged& event) {
        localPlayerId_ = event.player;
    });
    bus_.subscribe<events::SelectionChanged>(
        [this](const events::SelectionChanged& event) { onSelectionChanged(event); });
    subscribeUnits();
    subscribePlayers();
}

void WorldObjectsViewModel::drawBuildingPreview(
    model::BuildingType type, model::GridPos origin, int size, unsigned char alpha) const
{
    const BuildingViewModel preview{type, size, mineResourceViewModel_, commandCenterViewModel_,
        warehouseViewModel_, steelworksViewModel_, factoryViewModel_, barracksViewModel_};
    preview.drawAt(static_cast<float>(origin.x), static_cast<float>(origin.y), alpha);
}

void WorldObjectsViewModel::subscribeUnits()
{
    bus_.subscribe<events::UnitSpawned>([this](const events::UnitSpawned& event) {
        units_[event.entity] = UnitVisual{event.entity, event.x, event.y, event.size,
            std::make_unique<UnitViewModel>(event.type, event.size, event.health, event.maxHealth)};
    });
    bus_.subscribe<events::UnitRemoved>([this](const events::UnitRemoved& event) {
        units_.erase(event.entity);
        drones_.erase(event.entity);
        selectedUnits_.erase(event.entity);
    });
    bus_.subscribe<events::DroneSpawned>([this](const events::DroneSpawned& event) {
        drones_[event.entity] = DroneVisual{event.entity, event.x, event.y, std::make_unique<DroneViewModel>()};
    });
    bus_.subscribe<events::DroneMoved>([this](const events::DroneMoved& event) {
        const auto drone = drones_.find(event.entity);
        if (drone == drones_.end()) {
            return;
        }
        drone->second.x = event.x;
        drone->second.y = event.y;
    });
    bus_.subscribe<events::DroneLoadStateChanged>([this](const events::DroneLoadStateChanged& event) {
        const auto drone = drones_.find(event.entity);
        if (drone == drones_.end()) {
            return;
        }
        drone->second.viewModel->setLoaded(event.state == events::DroneLoadState::Loaded);
    });
    bus_.subscribe<events::UnitMoved>([this](const events::UnitMoved& event) {
        const auto unit = units_.find(event.entity);
        if (unit == units_.end()) {
            return;
        }
        unit->second.x = event.x;
        unit->second.y = event.y;
    });
    bus_.subscribe<events::HealthChanged>([this](const events::HealthChanged& event) {
        const auto unit = units_.find(event.entity);
        if (unit != units_.end()) {
            unit->second.viewModel->setHealth(event.health, event.maxHealth);
            return;
        }
        const auto building = std::find_if(buildings_.begin(), buildings_.end(),
            [&event](const BuildingVisual& candidate) { return candidate.entity == event.entity; });
        if (building == buildings_.end()) {
            return;
        }
        building->health = event.health;
        building->maxHealth = event.maxHealth;
    });
}

void WorldObjectsViewModel::subscribePlayers()
{
    bus_.subscribe<events::PlayerSpawned>([this](const events::PlayerSpawned& event) {
        players_[event.entity] = PlayerVisual{
            event.entity, event.player, event.x, event.y, std::make_unique<PlayerAvatarViewModel>()
        };
    });
    bus_.subscribe<events::PlayerMoved>([this](const events::PlayerMoved& event) {
        const auto player = players_.find(event.entity);
        if (player == players_.end()) {
            return;
        }
        player->second.x = event.x;
        player->second.y = event.y;
    });
}

void WorldObjectsViewModel::onSelectionChanged(const events::SelectionChanged& event)
{
    if (event.player != localPlayerId_) {
        return;
    }
    selectedUnits_ = std::unordered_set<model::ecs::Entity>(event.units.begin(), event.units.end());
    selectedBuilding_ = event.building;
}

std::optional<Vector2> WorldObjectsViewModel::playerGridPosition(model::PlayerId playerId) const
{
    for (const auto& [entity, player] : players_) {
        static_cast<void>(entity);
        if (player.player == playerId) {
            return Vector2{player.x, player.y};
        }
    }
    return std::nullopt;
}

void WorldObjectsViewModel::draw()
{
    shotEffects_.update(GetFrameTime());

    enum class Kind { Building, Unit, Drone, Player };
    struct Drawable {
        float depth;
        model::ecs::Entity entity;
        Kind kind;
        const WorldObjectViewModel* viewModel;
        const BuildingVisual* building;
        float x;
        float y;
        int size;
        model::GridPos origin;
    };

    std::vector<Drawable> objects;
    objects.reserve(buildings_.size() + units_.size() + drones_.size() + players_.size());
    for (const BuildingVisual& building : buildings_) {
        objects.push_back(Drawable{static_cast<float>(depthKey(building.origin, building.size)),
            building.entity, Kind::Building, building.viewModel.get(), &building, static_cast<float>(building.origin.x),
            static_cast<float>(building.origin.y), building.size, building.origin});
    }
    for (const auto& [entity, unit] : units_) {
        objects.push_back(Drawable{unit.x + unit.y, entity, Kind::Unit, unit.viewModel.get(), nullptr,
            unit.x, unit.y, unit.size, {}});
    }
    for (const auto& [entity, drone] : drones_) {
        objects.push_back(Drawable{drone.x + drone.y, entity, Kind::Drone, drone.viewModel.get(), nullptr,
            drone.x, drone.y, 1, {}});
    }
    for (const auto& [entity, player] : players_) {
        objects.push_back(Drawable{player.x + player.y, entity, Kind::Player, player.viewModel.get(), nullptr,
            player.x, player.y, 1, {}});
    }
    std::sort(objects.begin(), objects.end(), [](const Drawable& a, const Drawable& b) {
        return a.depth != b.depth ? a.depth < b.depth : a.entity < b.entity;
    });

    for (const Drawable& object : objects) {
        switch (object.kind) {
        case Kind::Building:
            if (selectedBuilding_ == object.entity) {
                constexpr float kMargin = 0.2f;
                const float x = static_cast<float>(object.origin.x) - kMargin;
                const float y = static_cast<float>(object.origin.y) - kMargin;
                const float size = static_cast<float>(object.size) + 2.0f * kMargin;
                iso::drawFootprint(x, y, size, size, kSelectedFill, kSelectedOutline);
            }
            break;
        case Kind::Unit:
            if (selectedUnits_.contains(object.entity)) {
                drawUnitSelectionRing(object.x, object.y, object.size);
            }
            break;
        case Kind::Drone:
        case Kind::Player:
            break;
        }
        object.viewModel->drawAt(object.x, object.y);
        if (object.building != nullptr && object.building->type == model::BuildingType::TrainingDummy
            && object.building->maxHealth > 0) {
            const float ratio = static_cast<float>(object.building->health)
                / static_cast<float>(object.building->maxHealth);
            drawUnitHealthBar(object.x + static_cast<float>(object.size) / 2.0F,
                object.y + static_cast<float>(object.size) / 2.0F, object.size, ratio);
        }
    }

    // Tracers stay on top of the world objects so short shots are still visible.
    shotEffects_.draw();
}

} // namespace view::gameplay
