#pragma once

#include "model/Player.hpp"

#include <string>

namespace model::components {

// Identifies a player entity. It intentionally has no spatial component.
struct PlayerProfile {
    PlayerId id;
    std::string name;
};

} // namespace model::components
