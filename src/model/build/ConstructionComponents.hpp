#pragma once

#include "model/build/BuildingType.hpp"
#include "model/economy/Item.hpp"
#include "model/ecs/Entity.hpp"

#include <array>
#include <cstddef>
#include <optional>

namespace model {

inline constexpr std::size_t kItemCount = static_cast<std::size_t>(ItemId::Count);
inline constexpr int kConstructionDroneCapacity = 10;
inline constexpr int kConstructionDroneIronCost = 50;
inline constexpr int kConstructionDroneElectronicsCost = 50;
inline constexpr float kConstructionDroneSpeed = 2.0F;

// Items the player may use to build. Weapons and ammo stay with the player.
inline constexpr std::array<ItemId, 6> kConstructionMaterials{
    ItemId::Iron, ItemId::Coal, ItemId::Steel, ItemId::Copper, ItemId::Electronics, ItemId::Tools};

} // namespace model

namespace model::components {

// A building shell that is not functional yet. It becomes a Building when construction finishes.
struct ConstructionSite {
    BuildingType type;
    float duration = 0.0F;
    float elapsed = 0.0F;
    std::array<int, kItemCount> delivered{};
    std::array<float, kItemCount> remainders{};
    // Items already loaded by a drone and on their way here, so two drones do not over-deliver.
    std::array<int, kItemCount> pending{};
    int publishedPercent = 0;
};

enum class ConstructionDroneState { Idle, ToSource, ToSite };

struct ConstructionDrone {
    ConstructionDroneState state = ConstructionDroneState::Idle;
    std::optional<ecs::Entity> site;
    std::optional<ecs::Entity> source;
    std::optional<ItemId> item;
};

} // namespace model::components
