#pragma once

#include <cstdint>

namespace model {

using PlayerId = std::uint32_t;

// Single player sandbox: the only player for now. Multiplayer will add more ids.
inline constexpr PlayerId kLocalPlayer = 0;

} // namespace model
