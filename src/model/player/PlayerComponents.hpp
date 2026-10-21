#pragma once

#include "model/player/Player.hpp"

#include <string>

namespace model::components {

// Identity and display name for a player's in-world avatar entity.
struct PlayerProfile {
    PlayerId id;
    std::string name;
};

} // namespace model::components
