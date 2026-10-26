#pragma once

#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/economy/Item.hpp"
#include "model/economy/Market.hpp"
#include "model/player/Player.hpp"

#include <unordered_map>

namespace model {

inline constexpr double kStartingCredit = 1000.0;
inline constexpr double kWeeklyInterestRate = 0.05;

// Per-player energy credit. It charges consumed resources at the current market price, credits
// sales, charges weekly interest on negative balances and owns the market that prices every item.
class EconomySystem {
public:
    explicit EconomySystem(core::EventBus& bus);

    void reset();

    // Opens an account with the starting credit and publishes its state.
    void initializePlayer(PlayerId player);

    [[nodiscard]] double balance(PlayerId player) const;
    [[nodiscard]] double weeklyInterest(PlayerId player) const;
    [[nodiscard]] bool debugFree() const;
    [[nodiscard]] const Market& market() const { return market_; }

    // Sells at the current market price, credits the owner and records the external demand.
    // Returns the revenue credited, or zero when nothing could be sold.
    double sell(PlayerId player, ItemId item, int quantity);

private:
    void onConsumed(const events::ResourceConsumed& event);
    void onProduced(const events::ResourceProduced& event);
    void onWeekStarted(const events::WeekStarted& event);

    void publishCredit(PlayerId player) const;
    void publishPrices() const;

    core::EventBus& bus_;
    Market market_;
    std::unordered_map<PlayerId, double> balances_;
    bool debugFree_ = false;
};

} // namespace model
