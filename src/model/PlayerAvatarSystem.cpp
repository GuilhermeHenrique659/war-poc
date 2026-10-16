#include "model/PlayerAvatarSystem.hpp"

#include "model/BuildingType.hpp"
#include "model/UnitType.hpp"
#include "model/components/BuildingComponents.hpp"
#include "model/components/Owner.hpp"
#include "model/components/PlayerComponents.hpp"

#include <algorithm>
#include <cmath>

namespace model {

namespace {

constexpr int kPlayerHealth = 10;
constexpr int kPlayerDamage = 1;
constexpr float kPlayerRange = 1.0F;
constexpr float kPlayerSpeed = unitSpec(UnitType::Cavalry).speed;
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
    entities_[player] = entity;
    return entity;
}

void PlayerAvatarSystem::move(PlayerId player, float directionX, float directionY, float deltaSeconds)
{
    const auto entity = entities_.find(player);
    if (entity == entities_.end() || deltaSeconds <= 0.0F) {
        return;
    }

    const float magnitude = std::hypot(directionX, directionY);
    if (magnitude <= 0.0F) {
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
    bus_.publish(events::PlayerMoved{entity->second, player, position->x, position->y});
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
        world_.emplace<components::Combat>(entity->second, kPlayerDamage, kPlayerRange);
        world_.emplace<components::Position>(entity->second, spawnPosition);
        spawnPositions_[entity->second] = spawnPosition;
        bus_.publish(events::PlayerSpawned{
            entity->second, event.owner, spawnPosition.x, spawnPosition.y, kPlayerHealth, kPlayerHealth});
        return;
    }

    spawnPositions_[entity->second] = spawnPosition;
    position->x = spawnPosition.x;
    position->y = spawnPosition.y;
    bus_.publish(events::PlayerMoved{entity->second, event.owner, position->x, position->y});
}

components::Position PlayerAvatarSystem::findSpawnPosition(GridPos origin, int size) const
{
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
                if (collidesWithBuilding(candidate)) {
                    continue;
                }

                const float centerX = static_cast<float>(origin.x) + static_cast<float>(size) / 2.0F;
                const float centerY = static_cast<float>(origin.y) + static_cast<float>(size) / 2.0F;
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
        const float left = static_cast<float>(footprint.origin.x);
        const float top = static_cast<float>(footprint.origin.y);
        const float right = left + static_cast<float>(footprint.size);
        const float bottom = top + static_cast<float>(footprint.size);
        collides = collides
            || (position.x + kPlayerCollisionRadius > left && position.x - kPlayerCollisionRadius < right
                && position.y + kPlayerCollisionRadius > top && position.y - kPlayerCollisionRadius < bottom);
    });
    return collides;
}

float PlayerAvatarSystem::moveAlongAxis(components::Position position, float distance, bool xAxis) const
{
    if (distance == 0.0F) {
        return xAxis ? position.x : position.y;
    }

    float destination = (xAxis ? position.x : position.y) + distance;
    world_.each<components::Footprint>([&](ecs::Entity, const components::Footprint& footprint) {
        const float left = static_cast<float>(footprint.origin.x);
        const float top = static_cast<float>(footprint.origin.y);
        const float right = left + static_cast<float>(footprint.size);
        const float bottom = top + static_cast<float>(footprint.size);
        const bool overlapsOtherAxis = xAxis
            ? position.y + kPlayerCollisionRadius > top && position.y - kPlayerCollisionRadius < bottom
            : position.x + kPlayerCollisionRadius > left && position.x - kPlayerCollisionRadius < right;
        if (!overlapsOtherAxis) {
            return;
        }

        const float current = xAxis ? position.x : position.y;
        const float obstacleStart = xAxis ? left : top;
        const float obstacleEnd = xAxis ? right : bottom;
        if (distance > 0.0F && current + kPlayerCollisionRadius <= obstacleStart
            && destination + kPlayerCollisionRadius > obstacleStart) {
            destination = std::min(destination, obstacleStart - kPlayerCollisionRadius);
        } else if (distance < 0.0F && current - kPlayerCollisionRadius >= obstacleEnd
            && destination - kPlayerCollisionRadius < obstacleEnd) {
            destination = std::max(destination, obstacleEnd + kPlayerCollisionRadius);
        }
    });
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
