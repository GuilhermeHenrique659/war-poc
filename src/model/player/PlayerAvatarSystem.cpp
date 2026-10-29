#include "model/player/PlayerAvatarSystem.hpp"

#include "model/FootprintGeometry.hpp"
#include "model/StateNotifier.hpp"
#include "model/build/BuildingType.hpp"
#include "model/combat/WeaponComponents.hpp"
#include "model/units/UnitType.hpp"
#include "model/components/CommonComponents.hpp"
#include "model/components/Owner.hpp"
#include "model/economy/InventoryComponents.hpp"
#include "model/economy/InventorySystem.hpp"
#include "model/economy/Item.hpp"
#include "model/player/PlayerComponents.hpp"
#include "model/units/UnitComponents.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace model {

namespace {

constexpr int kPlayerHealth = 10;
constexpr WeaponSpec kPlayerWeapon = weaponSpec(WeaponType::Pistol);
constexpr float kPlayerSpeed = unitSpec(model::UnitType::Engineer).speed;
constexpr float kPlayerCollisionRadius = 0.3F;

} // namespace

PlayerAvatarSystem::PlayerAvatarSystem(core::EventBus& bus, ecs::Registry& world)
    : bus_(bus)
    , world_(world)
{
    bus_.subscribe<events::BuildingPlaced>([this](const auto& event) { onBuildingPlaced(event); });
    bus_.subscribe<events::PlayerMoveRequested>([this](const auto& event) {
        move(event.player, event.directionX, event.directionY, event.deltaSeconds);
    });
    bus_.subscribe<events::PlayerRespawnRequested>([this](const auto& event) { respawn(event.entity); });
}

void PlayerAvatarSystem::reset()
{
    entities_.clear();
    spawnPositions_.clear();
}

ecs::Entity PlayerAvatarSystem::create(PlayerId player, const std::string& name)
{
    const ecs::Entity entity = world_.create();
    world_.emplace<components::PlayerProfile>(entity, player, name);
    world_.emplace<components::Owner>(entity, player);
    auto& inventory = world_.emplace<components::Inventory>(entity, std::size_t{32});
    (void)InventorySystem::add(inventory, ItemId::Iron, 500);
    (void)InventorySystem::add(inventory, ItemId::Weapon, 10);
    (void)InventorySystem::add(inventory, ItemId::Ammo, 1000);
    entities_[player] = entity;
    return entity;
}

std::optional<ecs::Entity> PlayerAvatarSystem::entity(PlayerId player) const
{
    const auto it = entities_.find(player);
    return it == entities_.end() ? std::nullopt : std::optional<ecs::Entity>{it->second};
}

void PlayerAvatarSystem::move(PlayerId player, float directionX, float directionY, float deltaSeconds)
{
    const auto entity = entities_.find(player);
    if (entity == entities_.end()) {
        return;
    }

    const float magnitude = std::hypot(directionX, directionY);
    if (magnitude <= 0.0F || deltaSeconds <= 0.0F) {
        updateState(entity->second, PlayerState::Idle);
        return;
    }

    auto* position = world_.tryGet<components::Position>(entity->second);
    if (position == nullptr) {
        return;
    }

    const float movementX = directionX / magnitude * kPlayerSpeed * deltaSeconds;
    const float movementY = directionY / magnitude * kPlayerSpeed * deltaSeconds;
    position->x = moveAlongAxis(*position, movementX, true);
    position->y = moveAlongAxis(*position, movementY, false);
    updateFacing(entity->second, directionX, directionY);
    updateState(entity->second, PlayerState::Moving);
    bus_.publish(events::PlayerMoved{entity->second, player, position->x, position->y});
}

void PlayerAvatarSystem::updateState(ecs::Entity entity, PlayerState state)
{
    // A player holding an attack order is engaging: the combat system owns its state.
    if (world_.has<components::AttackOrder>(entity)) {
        return;
    }
    setPlayerState(bus_, world_, entity, state);
}

void PlayerAvatarSystem::updateFacing(ecs::Entity entity, float directionX, float directionY)
{
    auto* facing = world_.tryGet<components::Facing>(entity);
    if (facing == nullptr) {
        return;
    }
    const Direction direction = directionFromGridMovement(directionX, directionY, facing->direction);
    if (direction == facing->direction) {
        return;
    }
    facing->direction = direction;
    bus_.publish(events::FacingChanged{entity, direction});
}

void PlayerAvatarSystem::onBuildingPlaced(const events::BuildingPlaced& event)
{
    if (event.type != BuildingType::CommandCenter) {
        return;
    }

    const auto entity = entities_.find(event.owner);
    if (entity == entities_.end()) {
        return;
    }

    const components::Position spawnPosition = findSpawnPosition(event.origin, event.size);
    auto* position = world_.tryGet<components::Position>(entity->second);
    if (position == nullptr) {
        world_.emplace<components::Health>(entity->second, kPlayerHealth, kPlayerHealth);
        world_.emplace<components::Weapon>(
            entity->second, WeaponType::Pistol, kPlayerWeapon.damage, kPlayerWeapon.range);
        world_.emplace<components::Position>(entity->second, spawnPosition);
        const Direction initialFacing
            = world_.emplace<components::Facing>(entity->second).direction;
        spawnPositions_[entity->second] = spawnPosition;
        bus_.publish(events::PlayerSpawned{
            entity->second, event.owner, spawnPosition.x, spawnPosition.y, kPlayerHealth, kPlayerHealth});
        bus_.publish(events::FacingChanged{entity->second, initialFacing});
        setPlayerState(bus_, world_, entity->second, PlayerState::Idle);
        return;
    }

    spawnPositions_[entity->second] = spawnPosition;
    position->x = spawnPosition.x;
    position->y = spawnPosition.y;
    bus_.publish(events::PlayerMoved{entity->second, event.owner, position->x, position->y});
}

components::Position PlayerAvatarSystem::findSpawnPosition(GridPos origin, int size) const
{
    const FootprintBounds target = footprintBounds(components::Footprint{origin, size});
    for (int ring = 0;; ++ring) {
        const int minX = origin.x - ring - 1;
        const int minY = origin.y - ring - 1;
        const int maxX = origin.x + size + ring;
        const int maxY = origin.y + size + ring;
        components::Position nearest{};
        float nearestDistance = 0.0F;
        bool found = false;

        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                if (x != minX && x != maxX && y != minY && y != maxY) {
                    continue;
                }

                const components::Position candidate{static_cast<float>(x) + 0.5F, static_cast<float>(y) + 0.5F};
                if (collidesWithBuilding(candidate) || !isPositionWalkable(candidate)) {
                    continue;
                }

                const float centerX = target.centerX();
                const float centerY = target.centerY();
                const float dx = candidate.x - centerX;
                const float dy = candidate.y - centerY;
                const float distance = dx * dx + dy * dy;
                if (!found || distance < nearestDistance) {
                    nearest = candidate;
                    nearestDistance = distance;
                    found = true;
                }
            }
        }

        if (found) {
            return nearest;
        }
    }
}

bool PlayerAvatarSystem::collidesWithBuilding(components::Position position) const
{
    bool collides = false;
    world_.each<components::Footprint>([&](ecs::Entity, const components::Footprint& footprint) {
        const FootprintBounds bounds = footprintBounds(footprint);
        collides = collides
            || (position.x + kPlayerCollisionRadius > bounds.left && position.x - kPlayerCollisionRadius < bounds.right
                && position.y + kPlayerCollisionRadius > bounds.top
                && position.y - kPlayerCollisionRadius < bounds.bottom);
    });
    return collides;
}

bool PlayerAvatarSystem::isPositionWalkable(components::Position position) const
{
    if (!tileWalkable_) {
        return true;
    }
    const int minX = static_cast<int>(std::floor(position.x - kPlayerCollisionRadius));
    const int minY = static_cast<int>(std::floor(position.y - kPlayerCollisionRadius));
    const int maxX = static_cast<int>(std::floor(position.x + kPlayerCollisionRadius - 0.001F));
    const int maxY = static_cast<int>(std::floor(position.y + kPlayerCollisionRadius - 0.001F));
    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            if (!tileWalkable_(GridPos{x, y})) {
                return false;
            }
        }
    }
    return true;
}

float PlayerAvatarSystem::moveAlongAxis(components::Position position, float distance, bool xAxis) const
{
    if (distance == 0.0F) {
        return xAxis ? position.x : position.y;
    }

    float destination = (xAxis ? position.x : position.y) + distance;
    world_.each<components::Footprint>([&](ecs::Entity, const components::Footprint& footprint) {
        const FootprintBounds bounds = footprintBounds(footprint);
        const bool overlapsOtherAxis = xAxis
            ? position.y + kPlayerCollisionRadius > bounds.top
                && position.y - kPlayerCollisionRadius < bounds.bottom
            : position.x + kPlayerCollisionRadius > bounds.left
                && position.x - kPlayerCollisionRadius < bounds.right;
        if (!overlapsOtherAxis) {
            return;
        }

        const float current = xAxis ? position.x : position.y;
        const float obstacleStart = xAxis ? bounds.left : bounds.top;
        const float obstacleEnd = xAxis ? bounds.right : bounds.bottom;
        if (distance > 0.0F && current + kPlayerCollisionRadius <= obstacleStart
            && destination + kPlayerCollisionRadius > obstacleStart) {
            destination = std::min(destination, obstacleStart - kPlayerCollisionRadius);
        } else if (distance < 0.0F && current - kPlayerCollisionRadius >= obstacleEnd
            && destination - kPlayerCollisionRadius < obstacleEnd) {
            destination = std::max(destination, obstacleEnd + kPlayerCollisionRadius);
        }
    });
    if (tileWalkable_) {
        const float current = xAxis ? position.x : position.y;
        const auto candidateAt = [&](float coordinate) {
            components::Position candidate = position;
            if (xAxis) {
                candidate.x = coordinate;
            } else {
                candidate.y = coordinate;
            }
            return candidate;
        };
        if (!isPositionWalkable(candidateAt(destination))) {
            float low = 0.0F;
            float high = 1.0F;
            for (int iteration = 0; iteration < 20; ++iteration) {
                const float middle = (low + high) / 2.0F;
                const float candidate = current + (destination - current) * middle;
                if (isPositionWalkable(candidateAt(candidate))) {
                    low = middle;
                } else {
                    high = middle;
                }
            }
            destination = current + (destination - current) * low;
        }
    }
    return destination;
}

void PlayerAvatarSystem::respawn(ecs::Entity entity)
{
    const auto start = spawnPositions_.find(entity);
    auto* position = world_.tryGet<components::Position>(entity);
    auto* health = world_.tryGet<components::Health>(entity);
    const auto* profile = world_.tryGet<components::PlayerProfile>(entity);
    if (start == spawnPositions_.end() || position == nullptr || health == nullptr || profile == nullptr) {
        return;
    }

    position->x = start->second.x;
    position->y = start->second.y;
    health->current = health->max;
    bus_.publish(events::HealthChanged{entity, health->current, health->max});
    bus_.publish(events::PlayerMoved{entity, profile->id, position->x, position->y});
}

} // namespace model
