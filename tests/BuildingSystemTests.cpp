#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/build/BuildingSystem.hpp"
#include "model/economy/InventoryComponents.hpp"
#include "model/economy/ProductionComponents.hpp"
#include "model/build/BuildingComponents.hpp"
#include "model/units/UnitComponents.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

namespace {

class BuildingSystemTest : public ::testing::Test {
protected:
    BuildingSystemTest()
        : buildings_(bus_, world_)
    {
        bus_.subscribe<events::BuildingPlaced>([this](const events::BuildingPlaced& event) {
            placements_.push_back(event);
        });
    }

    core::EventBus bus_;
    model::ecs::Registry world_;
    model::BuildingSystem buildings_;
    std::vector<events::BuildingPlaced> placements_;
    static constexpr model::PlayerId kPlayer = 1;
};

} // namespace

TEST_F(BuildingSystemTest, RequiresACommandCenterBeforeWallsCanBeBuilt)
{
    // Given a player who has not placed a Command Center.
    buildings_.select(kPlayer, model::BuildingType::Wall);

    // When the player attempts to place a wall.
    buildings_.place(kPlayer, {0, 0});

    // Then no wall is created.
    EXPECT_TRUE(placements_.empty());
}

TEST_F(BuildingSystemTest, GivesNewBuildingsCombatHealth)
{
    // Given a player placing a Command Center.
    buildings_.select(kPlayer, model::BuildingType::CommandCenter);

    // When the building is created.
    buildings_.place(kPlayer, {0, 0});

    // Then it has a full health pool for combat.
    ASSERT_EQ(placements_.size(), 1U);
    const auto* health = world_.tryGet<model::components::Health>(placements_.front().entity);
    ASSERT_NE(health, nullptr);
    EXPECT_EQ(health->current, 500);
    EXPECT_EQ(health->max, 500);
}

TEST_F(BuildingSystemTest, CreatesTheCommandCenterFootprintAndPreventsOverlap)
{
    // Given a player selecting their first Command Center.
    const model::GridPos anchor{10, 10};
    buildings_.select(kPlayer, model::BuildingType::CommandCenter);

    // When the player places it at the selected anchor.
    buildings_.place(kPlayer, anchor);

    // Then the complete 5 by 5 footprint is occupied by the owned building.
    ASSERT_EQ(placements_.size(), 1U);
    const events::BuildingPlaced commandCenter = placements_.front();
    EXPECT_EQ(commandCenter.owner, kPlayer);
    EXPECT_EQ(commandCenter.type, model::BuildingType::CommandCenter);
    EXPECT_EQ(commandCenter.size, 5);

    const model::GridPos expectedOrigin{8, 8};
    EXPECT_TRUE(commandCenter.origin == expectedOrigin);
    const auto* footprint = world_.tryGet<model::components::Footprint>(commandCenter.entity);
    ASSERT_NE(footprint, nullptr);
    EXPECT_TRUE(footprint->origin == expectedOrigin);
    EXPECT_EQ(footprint->size, 5);
    EXPECT_TRUE(buildings_.isOccupied(expectedOrigin));
    EXPECT_TRUE(buildings_.isOccupied(model::GridPos{12, 12}));

    // When the player tries to place a wall on top of that footprint.
    buildings_.select(kPlayer, model::BuildingType::Wall);
    buildings_.place(kPlayer, anchor);

    // Then the overlapping building is rejected.
    EXPECT_EQ(placements_.size(), 1U);
}

TEST_F(BuildingSystemTest, UnlocksWallsAfterTheCommandCenterIsBuilt)
{
    // Given a player who has already placed their Command Center.
    buildings_.select(kPlayer, model::BuildingType::CommandCenter);
    buildings_.place(kPlayer, {0, 0});

    // When the player selects and places a wall on a free tile.
    const model::GridPos wallTile{10, 10};
    buildings_.select(kPlayer, model::BuildingType::Wall);
    buildings_.place(kPlayer, wallTile);

    // Then the wall is created and reserves its tile.
    ASSERT_EQ(placements_.size(), 2U);
    const events::BuildingPlaced wall = placements_.back();
    EXPECT_EQ(wall.type, model::BuildingType::Wall);
    EXPECT_EQ(wall.size, 1);
    EXPECT_TRUE(wall.origin == wallTile);
    EXPECT_TRUE(buildings_.isOccupied(wallTile));
}

TEST_F(BuildingSystemTest, BuildingsAreFreeAndHaveInventorySlots)
{
    // Given a player with a completed Command Center and no construction costs.
    buildings_.select(kPlayer, model::BuildingType::CommandCenter);
    buildings_.place(kPlayer, {0, 0});

    // When the player places a mine.
    buildings_.select(kPlayer, model::BuildingType::IronMine);
    buildings_.place(kPlayer, {10, 10});

    // Then it is created for free with a 64-slot inventory and a primary production recipe.
    ASSERT_EQ(placements_.size(), 2U);
    const auto entity = placements_.back().entity;
    const auto* inventory = world_.tryGet<model::components::Inventory>(entity);
    const auto* production = world_.tryGet<model::components::Production>(entity);
    const auto* building = world_.tryGet<model::components::Building>(entity);
    ASSERT_NE(inventory, nullptr);
    ASSERT_NE(production, nullptr);
    ASSERT_NE(building, nullptr);
    EXPECT_EQ(inventory->slots.size(), 64U);
    EXPECT_TRUE(std::none_of(inventory->slots.begin(), inventory->slots.end(), [](const auto& slot) {
        return slot.has_value();
    }));
    EXPECT_EQ(building->name, "Mina de Ferro");
    EXPECT_TRUE(production->inputs.empty());
    EXPECT_EQ(production->output, model::ItemId::Iron);
    EXPECT_FLOAT_EQ(production->outputPerSecond, 1.0F);
}

TEST_F(BuildingSystemTest, CoalMineProducesOneCoalPerSecond)
{
    // Given a player with a completed Command Center.
    buildings_.select(kPlayer, model::BuildingType::CommandCenter);
    buildings_.place(kPlayer, {0, 0});

    // When the player places a Coal Mine.
    buildings_.select(kPlayer, model::BuildingType::CoalMine);
    buildings_.place(kPlayer, {10, 10});

    // Then it has a primary recipe for one coal per second.
    ASSERT_EQ(placements_.size(), 2U);
    const auto* production = world_.tryGet<model::components::Production>(placements_.back().entity);
    ASSERT_NE(production, nullptr);
    EXPECT_EQ(production->type(), model::ProductionType::Primary);
    EXPECT_TRUE(production->inputs.empty());
    EXPECT_EQ(production->output, model::ItemId::Coal);
    EXPECT_FLOAT_EQ(production->outputPerSecond, 1.0F);
}

TEST_F(BuildingSystemTest, SiderurgiaConsumesIronAndCoalAndProducesSteel)
{
    // Given a player with a completed Command Center.
    buildings_.select(kPlayer, model::BuildingType::CommandCenter);
    buildings_.place(kPlayer, {0, 0});

    // When the player builds a Siderurgia.
    buildings_.select(kPlayer, model::BuildingType::Steelworks);
    buildings_.place(kPlayer, {12, 12});

    // Then it has the defined input and output rates and a 3x3 footprint.
    ASSERT_EQ(placements_.size(), 2U);
    EXPECT_EQ(placements_.back().type, model::BuildingType::Steelworks);
    EXPECT_EQ(placements_.back().size, 3);
    const auto* production = world_.tryGet<model::components::Production>(placements_.back().entity);
    ASSERT_NE(production, nullptr);
    EXPECT_EQ(production->type(), model::ProductionType::Intermediate);
    ASSERT_EQ(production->inputs.size(), 2U);
    EXPECT_EQ(production->inputs[0].item, model::ItemId::Iron);
    EXPECT_FLOAT_EQ(production->inputs[0].quantityPerSecond, 1.0F);
    EXPECT_EQ(production->inputs[1].item, model::ItemId::Coal);
    EXPECT_FLOAT_EQ(production->inputs[1].quantityPerSecond, 1.0F);
    EXPECT_EQ(production->output, model::ItemId::Steel);
    EXPECT_FLOAT_EQ(production->outputPerSecond, 0.5F);
}
