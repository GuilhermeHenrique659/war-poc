#include "model/economy/ProductionSystem.hpp"

#include "core/Events.hpp"
#include "model/StateNotifier.hpp"
#include "model/components/Owner.hpp"
#include "model/economy/InventoryComponents.hpp"
#include "model/economy/InventorySystem.hpp"
#include "model/economy/ProductionComponents.hpp"

#include <algorithm>
#include <cmath>
#include <unordered_set>
#include <vector>

namespace model {

ProductionSystem::ProductionSystem(core::EventBus& bus)
    : bus_(bus)
{
}

void ProductionSystem::update(ecs::Registry& world, float deltaSeconds)
{
    if (!std::isfinite(deltaSeconds) || deltaSeconds <= 0.0F) {
        return;
    }

    std::unordered_set<ecs::Entity> producers;
    world.each<components::Production>([this, &world, &producers, deltaSeconds](
                                            ecs::Entity entity, const components::Production& production) {
        producers.insert(entity);
        const bool converting = produce(world, entity, production, deltaSeconds);
        setBuildingState(bus_, world, entity, converting ? BuildingState::Producing : BuildingState::Idle);
    });

    std::erase_if(progress_, [&producers](const auto& entry) { return !producers.contains(entry.first); });
}

bool ProductionSystem::produce(
    ecs::Registry& world, ecs::Entity entity, const components::Production& production, float deltaSeconds)
{
    auto* inventory = world.tryGet<components::Inventory>(entity);
    if (inventory == nullptr || !std::isfinite(production.outputPerSecond)
        || production.outputPerSecond <= 0.0F) {
        return false;
    }

    Progress& progress = progress_[entity];
    if (progress.inputRemainders.size() != production.inputs.size()) {
        progress.inputRemainders.assign(production.inputs.size(), 0.0F);
    }

    const auto* owner = world.tryGet<components::Owner>(entity);
    const PlayerId player = owner != nullptr ? owner->player : kNoPlayer;

    float productionTime = deltaSeconds;
    for (const ProductionInput& input : production.inputs) {
        if (!std::isfinite(input.quantityPerSecond) || input.quantityPerSecond <= 0.0F) {
            return false;
        }
        if (InventorySystem::amount(*inventory, input.item) == 0) {
            return false;
        }
        productionTime = std::min(productionTime, static_cast<float>(
            InventorySystem::amount(*inventory, input.item)) / input.quantityPerSecond);
    }
    productionTime = std::min(productionTime,
        static_cast<float>(InventorySystem::availableCapacity(*inventory, production.output))
            / production.outputPerSecond);
    if (productionTime <= 0.0F) {
        return false;
    }

    std::vector<int> inputConsumption;
    inputConsumption.reserve(production.inputs.size());
    for (std::size_t index = 0; index < production.inputs.size(); ++index) {
        const float accumulated
            = progress.inputRemainders[index] + production.inputs[index].quantityPerSecond * productionTime;
        const int wholeItems = static_cast<int>(std::floor(accumulated + 0.0001F));
        if (wholeItems > InventorySystem::amount(*inventory, production.inputs[index].item)) {
            return false;
        }
        inputConsumption.push_back(wholeItems);
    }
    const float accumulatedOutput = progress.outputRemainder + production.outputPerSecond * productionTime;
    const int outputItems = static_cast<int>(std::floor(accumulatedOutput + 0.0001F));
    if (outputItems > InventorySystem::availableCapacity(*inventory, production.output)) {
        return false;
    }

    for (std::size_t index = 0; index < production.inputs.size(); ++index) {
        const int consumed = inputConsumption[index];
        if (consumed > 0) {
            (void)InventorySystem::remove(*inventory, production.inputs[index].item, consumed);
            if (owner != nullptr) {
                bus_.publish(events::ResourceConsumed{player, production.inputs[index].item, consumed});
            }
        }
        const float accumulated
            = progress.inputRemainders[index] + production.inputs[index].quantityPerSecond * productionTime;
        progress.inputRemainders[index] = accumulated - static_cast<float>(consumed);
    }
    if (outputItems > 0) {
        (void)InventorySystem::add(*inventory, production.output, outputItems);
        if (owner != nullptr) {
            bus_.publish(events::ResourceProduced{player, production.output, outputItems});
        }
    }
    progress.outputRemainder = accumulatedOutput - static_cast<float>(outputItems);
    return true;
}

void ProductionSystem::reset()
{
    progress_.clear();
}

} // namespace model
