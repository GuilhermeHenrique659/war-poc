#pragma once

#include "model/economy/Item.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace model {

// Static trading parameters of one item. They never change during a match.
struct MarketDefinition {
    double basePrice = 0.0;
    int depth = 1;
    int weeklyQuota = 0;
};

// Item flows recorded during one week. They are summed into the next price at closeWeek().
struct MarketFlow {
    int produced = 0;
    int consumed = 0;
    int sold = 0;
};

[[nodiscard]] constexpr std::size_t itemIndex(ItemId item)
{
    return static_cast<std::size_t>(item);
}

// A week's supply and demand can move an item's price at most between these factors of its base.
inline constexpr double kMinMarketFactor = 0.5;
inline constexpr double kMaxMarketFactor = 2.0;

// Base prices, price sensitivity and the external demand each Trade Center places per week.
// Prices are in energy credits (EC) per unit.
[[nodiscard]] constexpr MarketDefinition marketDefinition(ItemId item)
{
    switch (item) {
    case ItemId::Iron: return {1.0, 500, 1000};
    case ItemId::Coal: return {1.0, 500, 800};
    case ItemId::Steel: return {5.0, 300, 500};
    case ItemId::Copper: return {1.0, 400, 600};
    case ItemId::Electronics: return {5.0, 150, 250};
    case ItemId::Tools: return {25.0, 80, 100};
    case ItemId::Weapon: return {80.0, 50, 500};
    case ItemId::Ammo: return {2.0, 500, 3000};
    case ItemId::Count: break;
    }
    return {};
}

// Weekly spot market for every resource. Within a week the price is fixed; when the week
// closes the recorded supply and demand move the prices for the following week.
class Market {
public:
    Market();

    [[nodiscard]] double price(ItemId item) const;

    [[nodiscard]] double basePrice(ItemId item) const;

    void recordProduction(ItemId item, int quantity);
    void recordConsumption(ItemId item, int quantity);
    void recordSale(ItemId item, int quantity);

    // Recomputes every price from the flows of the closing week and clears the counters.
    void closeWeek();

    [[nodiscard]] const MarketFlow& flows(ItemId item) const;

    void reset();

private:
    static constexpr std::size_t kItemCount = static_cast<std::size_t>(ItemId::Count);

    std::array<double, kItemCount> prices_{};
    std::array<MarketFlow, kItemCount> flows_{};
};

} // namespace model
