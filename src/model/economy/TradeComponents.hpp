#pragma once

#include "model/economy/Item.hpp"

#include <array>
#include <cstddef>

namespace model::components {

inline constexpr std::size_t kTradeItemCount = static_cast<std::size_t>(ItemId::Count);

// Quantity of each item a Trade Center already sold during the current week. The remaining
// external demand is the weekly quota minus this counter.
struct TradeQuota {
    std::array<int, kTradeItemCount> soldThisWeek{};
};

} // namespace model::components
