#include "model/EconomySystem.hpp"
#include "core/Events.hpp"

#include <algorithm>
#include <cmath>

namespace model {
namespace {
constexpr std::size_t indexOf(Resource resource) { return static_cast<std::size_t>(resource); }
}

void EconomySystem::initializePlayer(PlayerId player) { players_.try_emplace(player); publish(player); }
void EconomySystem::reset() { players_.clear(); }

void EconomySystem::update(float deltaSeconds)
{
    if (!std::isfinite(deltaSeconds) || deltaSeconds <= 0.0F) return;
    for (auto& [player, economy] : players_) {
        for (std::size_t i = 0; i < economy.stock.size(); ++i) {
            economy.stock[i] = std::min(economy.capacity[i], economy.stock[i] + economy.generation[i] * deltaSeconds);
        }
        publish(player);
    }
}

void EconomySystem::setCapacity(PlayerId player, Resource resource, int capacity)
{
    auto* economy = find(player);
    if (economy == nullptr || resource == Resource::Count) return;
    const auto i = indexOf(resource);
    economy->capacity[i] = static_cast<float>(std::max(0, capacity));
    economy->stock[i] = std::min(economy->stock[i], economy->capacity[i]);
    publish(player);
}

void EconomySystem::addGenerator(PlayerId player, Resource resource, float perSecond)
{
    auto* economy = find(player);
    if (economy == nullptr || resource == Resource::Count || !std::isfinite(perSecond)) return;
    economy->generation[indexOf(resource)] += std::max(0.0F, perSecond);
    publish(player);
}

void EconomySystem::addPopulationCapacity(PlayerId player, int amount)
{
    if (auto* economy = find(player); economy != nullptr) { economy->populationCapacity += std::max(0, amount); publish(player); }
}

void EconomySystem::applyBuilding(PlayerId player, BuildingType type)
{
    switch (type) {
    case BuildingType::House: addPopulationCapacity(player, 5); break;
    case BuildingType::Generator:
        addGenerator(player, Resource::Gold, 2.0F);
        addGenerator(player, Resource::Wood, 1.0F);
        addGenerator(player, Resource::Steel, 0.25F);
        addGenerator(player, Resource::Oil, 0.25F);
        break;
    case BuildingType::Warehouse:
        for (Resource resource : {Resource::Gold, Resource::Wood, Resource::Steel, Resource::Oil}) {
            setCapacity(player, resource, capacity(player, resource) + 250);
        }
        break;
    case BuildingType::CommandCenter: addPopulationCapacity(player, 5); break;
    case BuildingType::Wall:
    case BuildingType::Barracks:
    case BuildingType::Tower: break;
    }
}

bool EconomySystem::canReservePopulation(PlayerId player, int amount) const
{
    const auto* economy = find(player);
    return economy != nullptr && amount >= 0 && economy->populationUsed + amount <= economy->populationCapacity;
}

bool EconomySystem::reservePopulation(PlayerId player, int amount)
{
    if (!canReservePopulation(player, amount)) return false;
    find(player)->populationUsed += amount;
    publish(player);
    return true;
}

void EconomySystem::releasePopulation(PlayerId player, int amount)
{
    if (auto* economy = find(player); economy != nullptr) { economy->populationUsed = std::max(0, economy->populationUsed - std::max(0, amount)); publish(player); }
}

int EconomySystem::amount(PlayerId player, Resource resource) const
{
    const auto* economy = find(player);
    if (economy == nullptr || resource == Resource::Count) return 0;
    return static_cast<int>(economy->stock[indexOf(resource)]);
}

int EconomySystem::capacity(PlayerId player, Resource resource) const
{
    const auto* economy = find(player);
    if (economy == nullptr || resource == Resource::Count) return 0;
    return static_cast<int>(economy->capacity[indexOf(resource)]);
}

int EconomySystem::populationUsed(PlayerId player) const { const auto* e = find(player); return e == nullptr ? 0 : e->populationUsed; }
int EconomySystem::populationCapacity(PlayerId player) const { const auto* e = find(player); return e == nullptr ? 0 : e->populationCapacity; }

bool EconomySystem::canAfford(PlayerId player, ResourceCost cost) const
{
    const auto* e = find(player);
    if (e == nullptr) return false;
    const std::array<int, 4> costs{cost.gold, cost.wood, cost.steel, cost.oil};
    for (std::size_t i = 0; i < costs.size(); ++i) if (costs[i] < 0 || e->stock[i] < static_cast<float>(costs[i])) return false;
    return true;
}

bool EconomySystem::spend(PlayerId player, ResourceCost cost)
{
    if (!canAfford(player, cost)) return false;
    auto* e = find(player);
    const std::array<int, 4> costs{cost.gold, cost.wood, cost.steel, cost.oil};
    for (std::size_t i = 0; i < costs.size(); ++i) e->stock[i] -= static_cast<float>(costs[i]);
    publish(player);
    return true;
}

const EconomySystem::PlayerEconomy* EconomySystem::find(PlayerId player) const
{
    const auto it = players_.find(player);
    return it == players_.end() ? nullptr : &it->second;
}
EconomySystem::PlayerEconomy* EconomySystem::find(PlayerId player)
{
    const auto it = players_.find(player);
    return it == players_.end() ? nullptr : &it->second;
}

void EconomySystem::publish(PlayerId player) const
{
    const PlayerEconomy* economy = find(player);
    if (bus_ == nullptr || economy == nullptr) return;
    std::array<int, 4> stock{};
    std::array<int, 4> capacity{};
    for (std::size_t i = 0; i < stock.size(); ++i) {
        stock[i] = static_cast<int>(economy->stock[i]);
        capacity[i] = static_cast<int>(economy->capacity[i]);
    }
    bus_->publish(events::EconomyChanged{player, stock, capacity, economy->populationUsed, economy->populationCapacity});
}
} // namespace model
