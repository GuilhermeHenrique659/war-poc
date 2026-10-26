#include "model/economy/Market.hpp"

#include <algorithm>

namespace model {

namespace {

[[nodiscard]] bool isValid(ItemId item)
{
    return itemIndex(item) < static_cast<std::size_t>(ItemId::Count);
}

} // namespace

Market::Market()
{
    reset();
}

void Market::reset()
{
    flows_ = {};
    for (const ItemId item : kAllItems) {
        prices_[itemIndex(item)] = marketDefinition(item).basePrice;
    }
}

double Market::price(ItemId item) const
{
    if (!isValid(item)) {
        return 0.0;
    }
    return prices_[itemIndex(item)];
}

double Market::basePrice(ItemId item) const
{
    return marketDefinition(item).basePrice;
}

void Market::recordProduction(ItemId item, int quantity)
{
    if (!isValid(item) || quantity <= 0) {
        return;
    }
    flows_[itemIndex(item)].produced += quantity;
}

void Market::recordConsumption(ItemId item, int quantity)
{
    if (!isValid(item) || quantity <= 0) {
        return;
    }
    flows_[itemIndex(item)].consumed += quantity;
}

void Market::recordSale(ItemId item, int quantity)
{
    if (!isValid(item) || quantity <= 0) {
        return;
    }
    flows_[itemIndex(item)].sold += quantity;
}

void Market::closeWeek()
{
    for (const ItemId item : kAllItems) {
        const MarketFlow flow = flows_[itemIndex(item)];
        const MarketDefinition definition = marketDefinition(item);
        const double demand = static_cast<double>(flow.consumed + flow.sold);
        const double supply = static_cast<double>(flow.produced);
        const double depth = static_cast<double>(definition.depth);
        const double pressure = (demand + depth) / (supply + depth);
        const double factor = std::clamp(pressure, kMinMarketFactor, kMaxMarketFactor);
        prices_[itemIndex(item)] = definition.basePrice * factor;
    }
    flows_ = {};
}

const MarketFlow& Market::flows(ItemId item) const
{
    static const MarketFlow kEmpty{};
    if (!isValid(item)) {
        return kEmpty;
    }
    return flows_[itemIndex(item)];
}

} // namespace model
