#pragma once

#include <array>
#include <string_view>

namespace model {

enum class UnitType { Infantry, Ranged, Cavalry };

// Order in which the recruit panel lists the units.
inline constexpr std::array kUnitTypes{UnitType::Infantry, UnitType::Ranged, UnitType::Cavalry};

// Combat stats (health, damage, range) are stored but not used until combat exists.
struct UnitSpec {
    std::string_view label;
    int maxHealth;
    int damage;
    float range; // Tiles.
    float speed; // Tiles per second.
};

[[nodiscard]] constexpr UnitSpec unitSpec(UnitType type)
{
    switch (type) {
    case UnitType::Infantry:
        return UnitSpec{"Infantaria", 100, 10, 1.0f, 2.0f};
    case UnitType::Ranged:
        return UnitSpec{"Longa Distancia", 60, 8, 5.0f, 1.6f};
    case UnitType::Cavalry:
        return UnitSpec{"Cavalaria", 140, 14, 1.0f, 3.5f};
    }
    return UnitSpec{"?", 1, 0, 0.0f, 1.0f};
}

} // namespace model
