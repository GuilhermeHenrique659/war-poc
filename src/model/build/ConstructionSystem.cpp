#include "model/build/ConstructionSystem.hpp"

#include "core/Events.hpp"
#include "model/FootprintGeometry.hpp"
#include "model/build/BuildingFactory.hpp"
#include "model/build/ConstructionComponents.hpp"
#include "model/components/Owner.hpp"
#include "model/economy/InventoryComponents.hpp"
#include "model/economy/InventorySystem.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace model {

namespace {

// Float accumulation of frame deltas can stop a hair short of the nominal duration.
constexpr float kCompletionEpsilon = 1.0e-4F;

[[nodiscard]] float rateFor(const BuildingCost& cost, float duration)
{
    if (cost.quantity <= 0 || duration <= 0.0F) {
        return 0.0F;
    }
    return static_cast<float>(cost.quantity) / duration;
}

} // namespace

ConstructionSystem::ConstructionSystem(core::EventBus& bus, ecs::Registry& world)
    : bus_(bus)
    , world_(world)
{
    bus_.subscribe<events::DebugToggleRequested>(
        [this](const events::DebugToggleRequested&) { debugMode_ = !debugMode_; });
}

void ConstructionSystem::reset()
{
}

void ConstructionSystem::update(float deltaSeconds)
{
    if (!std::isfinite(deltaSeconds) || deltaSeconds <= 0.0F) {
        return;
    }

    std::vector<ecs::Entity> sites;
    world_.each<components::ConstructionSite>(
        [&sites](ecs::Entity entity, const components::ConstructionSite&) { sites.push_back(entity); });
    for (const ecs::Entity entity : sites) {
        if (auto* site = world_.tryGet<components::ConstructionSite>(entity)) {
            updateSite(entity, *site, deltaSeconds);
        }
    }
}

void ConstructionSystem::updateSite(ecs::Entity entity, components::ConstructionSite& site, float deltaSeconds)
{
    if (site.duration <= 0.0F) {
        completeSite(entity, site);
        return;
    }

    float workTime = std::min(deltaSeconds, site.duration - site.elapsed);
    auto* inventory = world_.tryGet<components::Inventory>(entity);
    const BuildingCosts& costs = buildingCosts(site.type);
    const auto* owner = world_.tryGet<components::Owner>(entity);
    const PlayerId player = owner != nullptr ? owner->player : kNoPlayer;

    if (!debugMode_ && inventory != nullptr) {
        for (const BuildingCost& cost : costs) {
            const float rate = rateFor(cost, site.duration);
            if (rate <= 0.0F) {
                continue;
            }
            const float available = static_cast<float>(InventorySystem::amount(*inventory, cost.item));
            workTime = std::min(workTime, available / rate);
        }
    }

    if (workTime <= 0.0F) {
        return;
    }

    if (!debugMode_ && inventory != nullptr) {
        for (const BuildingCost& cost : costs) {
            const float rate = rateFor(cost, site.duration);
            if (rate <= 0.0F) {
                continue;
            }
            const auto index = static_cast<std::size_t>(cost.item);
            float& remainder = site.remainders[index];
            const float total = remainder + rate * workTime;
            int whole = static_cast<int>(std::floor(total + 1.0e-4F));
            whole = std::min(whole, InventorySystem::amount(*inventory, cost.item));
            remainder = std::max(0.0F, total - static_cast<float>(whole));
            if (whole > 0) {
                (void)InventorySystem::remove(*inventory, cost.item, whole);
                if (owner != nullptr) {
                    bus_.publish(events::ResourceConsumed{player, cost.item, whole});
                }
            }
        }
    }

    site.elapsed = std::min(site.duration, site.elapsed + workTime);
    const float progress = site.elapsed / site.duration;
    const int percent = static_cast<int>(std::floor(progress * 100.0F));
    if (percent != site.publishedPercent) {
        site.publishedPercent = percent;
        bus_.publish(events::ConstructionProgressChanged{entity, progress});
    }
    if (site.elapsed >= site.duration - kCompletionEpsilon) {
        completeSite(entity, site);
    }
}

void ConstructionSystem::completeSite(ecs::Entity entity, const components::ConstructionSite& site)
{
    const auto* footprint = world_.tryGet<components::Footprint>(entity);
    const auto* owner = world_.tryGet<components::Owner>(entity);
    if (footprint == nullptr || owner == nullptr) {
        world_.destroy(entity);
        return;
    }

    const BuildingType type = site.type;
    const GridPos origin = footprint->origin;
    const int size = footprint->size;
    const PlayerId player = owner->player;

    world_.remove<components::ConstructionSite>(entity);
    completeBuilding(world_, entity, type);
    bus_.publish(events::ConstructionProgressChanged{entity, 1.0F});
    bus_.publish(events::BuildingPlaced{entity, player, type, origin, size});
}

} // namespace model
