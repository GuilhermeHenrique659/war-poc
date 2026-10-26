#include "model/economy/EconomySystem.hpp"

#include <cmath>

namespace model {

namespace {

[[nodiscard]] double interestFor(double balance)
{
    if (balance >= 0.0) {
        return 0.0;
    }
    return std::ceil(-balance * kWeeklyInterestRate);
}

[[nodiscard]] bool isValidItem(ItemId item)
{
    return itemIndex(item) < static_cast<std::size_t>(ItemId::Count);
}

} // namespace

EconomySystem::EconomySystem(core::EventBus& bus)
    : bus_(bus)
{
    bus_.subscribe<events::ResourceConsumed>(
        [this](const events::ResourceConsumed& event) { onConsumed(event); });
    bus_.subscribe<events::ResourceProduced>(
        [this](const events::ResourceProduced& event) { onProduced(event); });
    bus_.subscribe<events::WeekStarted>(
        [this](const events::WeekStarted& event) { onWeekStarted(event); });
    bus_.subscribe<events::DebugToggleRequested>(
        [this](const events::DebugToggleRequested&) { debugFree_ = !debugFree_; });
}

void EconomySystem::reset()
{
    balances_.clear();
    market_.reset();
    debugFree_ = false;
}

void EconomySystem::initializePlayer(PlayerId player)
{
    balances_[player] = kStartingCredit;
    publishCredit(player);
}

double EconomySystem::balance(PlayerId player) const
{
    const auto it = balances_.find(player);
    return it == balances_.end() ? 0.0 : it->second;
}

double EconomySystem::weeklyInterest(PlayerId player) const
{
    return interestFor(balance(player));
}

bool EconomySystem::debugFree() const
{
    return debugFree_;
}

double EconomySystem::sell(PlayerId player, ItemId item, int quantity)
{
    if (quantity <= 0 || !isValidItem(item) || !balances_.contains(player)) {
        return 0.0;
    }

    const double unitPrice = market_.price(item);
    const double revenue = unitPrice * static_cast<double>(quantity);
    market_.recordSale(item, quantity);
    balances_[player] += revenue;
    bus_.publish(events::TradeCompleted{player, item, quantity, unitPrice, revenue});
    publishCredit(player);
    return revenue;
}

void EconomySystem::onConsumed(const events::ResourceConsumed& event)
{
    if (event.quantity <= 0 || !isValidItem(event.item)) {
        return;
    }

    market_.recordConsumption(event.item, event.quantity);
    const auto it = balances_.find(event.player);
    if (it == balances_.end() || debugFree_) {
        return;
    }

    it->second -= market_.price(event.item) * static_cast<double>(event.quantity);
    publishCredit(event.player);
}

void EconomySystem::onProduced(const events::ResourceProduced& event)
{
    if (event.quantity <= 0 || !isValidItem(event.item)) {
        return;
    }
    market_.recordProduction(event.item, event.quantity);
}

void EconomySystem::onWeekStarted(const events::WeekStarted& /*event*/)
{
    for (auto& [player, balance] : balances_) {
        const double interest = interestFor(balance);
        if (interest > 0.0) {
            balance -= interest;
        }
        publishCredit(player);
    }
    market_.closeWeek();
    publishPrices();
}

void EconomySystem::publishCredit(PlayerId player) const
{
    bus_.publish(events::CreditChanged{player, balance(player), weeklyInterest(player)});
}

void EconomySystem::publishPrices() const
{
    events::MarketPricesChanged event{};
    for (const ItemId item : kAllItems) {
        event.prices[itemIndex(item)] = market_.price(item);
    }
    bus_.publish(event);
}

} // namespace model
