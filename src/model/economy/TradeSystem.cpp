#include "model/economy/TradeSystem.hpp"

#include "core/Events.hpp"
#include "model/build/BuildingComponents.hpp"
#include "model/build/BuildingType.hpp"
#include "model/components/Owner.hpp"
#include "model/economy/InventoryComponents.hpp"
#include "model/economy/InventorySystem.hpp"
#include "model/economy/Market.hpp"
#include "model/economy/TradeComponents.hpp"

#include <algorithm>
#include <vector>

namespace model {

TradeSystem::TradeSystem(core::EventBus& bus, ecs::Registry& world, EconomySystem& economy)
    : bus_(bus)
    , world_(world)
    , economy_(economy)
{
    bus_.subscribe<events::WeekStarted>([this](const events::WeekStarted&) { onWeekStarted(); });
}

void TradeSystem::reset()
{
}

void TradeSystem::update()
{
    std::vector<ecs::Entity> centers;
    world_.each<components::Building>([&centers](ecs::Entity entity, const components::Building& building) {
        if (building.type == BuildingType::TradeCenter) {
            centers.push_back(entity);
        }
    });
    for (const ecs::Entity center : centers) {
        sellFrom(center);
    }
}

void TradeSystem::publishPanel(PlayerId player, ecs::Entity center) const
{
    events::TradePanelChanged event{};
    event.player = player;

    const auto* building = world_.tryGet<components::Building>(center);
    const auto* owner = world_.tryGet<components::Owner>(center);
    if (building == nullptr || building->type != BuildingType::TradeCenter || owner == nullptr
        || owner->player != player) {
        bus_.publish(event);
        return;
    }

    event.open = true;
    event.building = center;
    const auto* inventory = world_.tryGet<components::Inventory>(center);
    event.rows.reserve(kAllItems.size());
    for (const ItemId item : kAllItems) {
        event.rows.push_back(events::TradeRow{
            item,
            economy_.market().price(item),
            remainingQuota(center, item),
            weeklyQuota(item),
            inventory != nullptr ? InventorySystem::amount(*inventory, item) : 0});
    }
    bus_.publish(event);
}

void TradeSystem::closePanel(PlayerId player) const
{
    events::TradePanelChanged closed{};
    closed.player = player;
    bus_.publish(closed);
}

int TradeSystem::weeklyQuota(ItemId item) const
{
    return marketDefinition(item).weeklyQuota;
}

int TradeSystem::remainingQuota(ecs::Entity center, ItemId item) const
{
    const auto* quota = world_.tryGet<components::TradeQuota>(center);
    const int sold = quota != nullptr ? quota->soldThisWeek[itemIndex(item)] : 0;
    return std::max(0, weeklyQuota(item) - sold);
}

void TradeSystem::sellFrom(ecs::Entity center)
{
    const auto* owner = world_.tryGet<components::Owner>(center);
    auto* inventory = world_.tryGet<components::Inventory>(center);
    auto* quota = world_.tryGet<components::TradeQuota>(center);
    if (owner == nullptr || inventory == nullptr || quota == nullptr) {
        return;
    }

    for (const ItemId item : kAllItems) {
        const int quantity = std::min(InventorySystem::amount(*inventory, item), remainingQuota(center, item));
        if (quantity <= 0) {
            continue;
        }
        const int removed = InventorySystem::remove(*inventory, item, quantity);
        if (removed <= 0) {
            continue;
        }
        // Selling requires an open credit account; otherwise the goods stay in the building.
        if (economy_.sell(owner->player, item, removed) <= 0.0) {
            (void)InventorySystem::add(*inventory, item, removed);
            continue;
        }
        quota->soldThisWeek[itemIndex(item)] += removed;
    }
}

void TradeSystem::onWeekStarted()
{
    world_.each<components::TradeQuota>(
        [](ecs::Entity, components::TradeQuota& quota) { quota.soldThisWeek = {}; });
}

} // namespace model
