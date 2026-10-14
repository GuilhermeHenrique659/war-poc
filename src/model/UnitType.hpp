#pragma once

#include <algorithm>
#include <array>
#include <string_view>

namespace model {

// Combined is the result of merging units; it cannot be recruited.
enum class UnitType { Infantry, Ranged, Cavalry, Combined };

// Order in which the recruit panel lists the units.
inline constexpr std::array kRecruitableUnitTypes{UnitType::Infantry, UnitType::Ranged, UnitType::Cavalry};

// Combat stats (health, damage, range) are stored but not used until combat exists.
struct UnitSpec {
    std::string_view label;
    int maxHealth;
    int damage;
    float range; // Tiles.
    float speed; // Tiles per second.
};

// Combined units take their stats from the merge, so their spec only carries the label.
[[nodiscard]] constexpr UnitSpec unitSpec(UnitType type)
{
    switch (type) {
    case UnitType::Infantry:
        return UnitSpec{"Infantaria", 100, 10, 1.0f, 2.0f};
    case UnitType::Ranged:
        return UnitSpec{"Longa Distancia", 60, 8, 5.0f, 1.6f};
    case UnitType::Cavalry:
        return UnitSpec{"Cavalaria", 140, 14, 1.0f, 3.5f};
    case UnitType::Combined:
        return UnitSpec{"Tropa Combinada", 0, 0, 0.0f, 0.0f};
    }
    return UnitSpec{"?", 1, 0, 0.0f, 1.0f};
}

// Everything that defines one unit instance.
struct UnitStats {
    UnitType type;
    int health;
    int maxHealth;
    int damage;
    float range;
    float speed;
    int baseCount; // Recruited units inside it (1 for a recruited unit).
    int size; // Square footprint, in tiles per side.
};

inline constexpr int kMaxUnitSize = 5;
inline constexpr int kPowerPerUnitSize = 400;

[[nodiscard]] constexpr int unitPower(int maxHealth, int damage)
{
    return maxHealth + 10 * damage;
}

// Every kPowerPerUnitSize of power adds one tile per side.
[[nodiscard]] constexpr int unitSizeForPower(int power)
{
    return std::clamp(1 + power / kPowerPerUnitSize, 1, kMaxUnitSize);
}

[[nodiscard]] constexpr UnitStats recruitStats(UnitType type)
{
    const UnitSpec spec = unitSpec(type);
    return UnitStats{type, spec.maxHealth, spec.maxHealth, spec.damage, spec.range, spec.speed, 1,
        unitSizeForPower(unitPower(spec.maxHealth, spec.damage))};
}

} // namespace model
