#pragma once

#include "core/EventBus.hpp"
#include "model/economy/EconomySystem.hpp"
#include "model/economy/Item.hpp"
#include "model/ecs/Entity.hpp"
#include "model/ecs/Registry.hpp"
#include "model/player/Player.hpp"

namespace model {

// External demand for resources: every Trade Center sells what it stores, up to its weekly quota
// per item, at the current market price. The quota renews when a new week begins.
class TradeSystem {
public:
    TradeSystem(core::EventBus& bus, ecs::Registry& world, EconomySystem& economy);

    void reset();

    // Sells the stored items of every Trade Center, limited by the remaining weekly quota.
    void update();

    // Publishes the panel data for the selected Trade Center, or a closed panel when the entity is
    // not a Trade Center owned by the player.
    void publishPanel(PlayerId player, ecs::Entity center) const;
    void closePanel(PlayerId player) const;

    [[nodiscard]] int weeklyQuota(ItemId item) const;
    [[nodiscard]] int remainingQuota(ecs::Entity center, ItemId item) const;

private:
    void sellFrom(ecs::Entity center);
    void onWeekStarted();

    core::EventBus& bus_;
    ecs::Registry& world_;
    EconomySystem& economy_;
};

} // namespace model
