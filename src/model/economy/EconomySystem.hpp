#pragma once

#include "core/EventBus.hpp"
#include "model/player/Player.hpp"
#include "model/build/BuildingType.hpp"
#include "model/units/UnitType.hpp"

#include <array>
#include <unordered_map>

namespace model {

enum class Resource : unsigned char { Gold, Wood, Steel, Oil, Count };

struct ResourceCost {
    int gold = 0;
    int wood = 0;
    int steel = 0;
    int oil = 0;
};

[[nodiscard]] constexpr ResourceCost buildingCost(BuildingType type)
{
    switch (type) {
    case BuildingType::CommandCenter: return {};
    case BuildingType::Wall: return {0, 5, 0, 0};
    case BuildingType::House: return {20, 20, 0, 0};
    case BuildingType::Generator: return {30, 10, 0, 0};
    case BuildingType::Warehouse: return {25, 15, 0, 0};
    case BuildingType::Barracks: return {50, 30, 10, 0};
    case BuildingType::Cruzador: return {50, 30, 10, 0};
    case BuildingType::Tower: return {40, 20, 5, 0};
    }
    return {};
}

[[nodiscard]] constexpr ResourceCost recruitmentCost(UnitType type)
{
    switch (type) {
    case UnitType::Soldier: return {50, 0, 0, 0};
    case UnitType::Bat: return {40, 10, 0, 0};
    case UnitType::Goat: return {75, 0, 5, 0};
    case UnitType::Combined: return {};
    }
    return {};
}

class EconomySystem {
public:
    explicit EconomySystem(core::EventBus* bus = nullptr) : bus_(bus) {}
    void initializePlayer(PlayerId player);
    void reset();
    void update(float deltaSeconds);
    void setCapacity(PlayerId player, Resource resource, int capacity);
    void addGenerator(PlayerId player, Resource resource, float perSecond);
    void addPopulationCapacity(PlayerId player, int amount);
    void applyBuilding(PlayerId player, BuildingType type);
    [[nodiscard]] bool canReservePopulation(PlayerId player, int amount) const;
    bool reservePopulation(PlayerId player, int amount);
    void releasePopulation(PlayerId player, int amount);
    [[nodiscard]] int amount(PlayerId player, Resource resource) const;
    [[nodiscard]] int capacity(PlayerId player, Resource resource) const;
    [[nodiscard]] int populationUsed(PlayerId player) const;
    [[nodiscard]] int populationCapacity(PlayerId player) const;
    [[nodiscard]] bool canAfford(PlayerId player, ResourceCost cost) const;
    bool spend(PlayerId player, ResourceCost cost);

private:
    struct PlayerEconomy {
        std::array<float, 4> stock{100.0F, 100.0F, 25.0F, 10.0F};
        std::array<float, 4> capacity{500.0F, 500.0F, 500.0F, 500.0F};
        std::array<float, 4> generation{};
        int populationCapacity = 10;
        int populationUsed = 0;
    };
    [[nodiscard]] const PlayerEconomy* find(PlayerId player) const;
    [[nodiscard]] PlayerEconomy* find(PlayerId player);
    void publish(PlayerId player) const;
    core::EventBus* bus_ = nullptr;
    std::unordered_map<PlayerId, PlayerEconomy> players_;
};

} // namespace model
