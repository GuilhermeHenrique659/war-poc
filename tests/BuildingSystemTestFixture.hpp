#pragma once

#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/build/BuildingSystem.hpp"
#include "model/economy/InventoryComponents.hpp"
#include "model/economy/InventorySystem.hpp"
#include "model/economy/Item.hpp"
#include "model/economy/ProductionComponents.hpp"
#include "model/build/BuildingComponents.hpp"
#include "model/components/Owner.hpp"
#include "model/player/PlayerComponents.hpp"
#include "model/units/UnitComponents.hpp"

#include <gtest/gtest.h>

#include <vector>

namespace buildingsystem_test {

// Shared fixture for BuildingSystem tests: a world with one player avatar that owns an inventory
// and records every event the system publishes.
class BuildingSystemTest : public ::testing::Test {
protected:
    BuildingSystemTest()
        : buildings_(bus_, world_)
    {
        bus_.subscribe<events::BuildingPlaced>([this](const events::BuildingPlaced& event) {
            placements_.push_back(event);
        });
        bus_.subscribe<events::BuildPreviewChanged>([this](const events::BuildPreviewChanged& event) {
            previews_.push_back(event);
        });
        bus_.subscribe<events::BuildBarChanged>([this](const events::BuildBarChanged& event) {
            buildBars_.push_back(event);
        });
        const auto playerEntity = world_.create();
        world_.emplace<model::components::PlayerProfile>(playerEntity, kPlayer, "Tester");
        world_.emplace<model::components::Owner>(playerEntity, kPlayer);
        playerInventory_ = &world_.emplace<model::components::Inventory>(playerEntity, std::size_t{32});
        (void)model::InventorySystem::add(*playerInventory_, model::ItemId::Iron, 500);
    }

    void addResource(model::ItemId item, int quantity)
    {
        EXPECT_EQ(model::InventorySystem::add(*playerInventory_, item, quantity), quantity);
    }

    core::EventBus bus_;
    model::ecs::Registry world_;
    model::BuildingSystem buildings_;
    std::vector<events::BuildingPlaced> placements_;
    std::vector<events::BuildPreviewChanged> previews_;
    std::vector<events::BuildBarChanged> buildBars_;
    model::components::Inventory* playerInventory_ = nullptr;
    static constexpr model::PlayerId kPlayer = 1;
};

} // namespace buildingsystem_test
