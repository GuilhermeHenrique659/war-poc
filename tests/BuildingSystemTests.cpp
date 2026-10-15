#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/BuildingSystem.hpp"
#include "model/EconomySystem.hpp"
#include "model/components/BuildingComponents.hpp"
#include "model/components/UnitComponents.hpp"

#include <gtest/gtest.h>

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

TEST_F(BuildingSystemTest, ChargesBuildingCostsAndAppliesHousePopulationCapacity)
{
    // Given a player with an initialized economy and a completed Command Center.
    model::EconomySystem economy;
    buildings_.setEconomy(&economy);
    buildings_.initializePlayer(kPlayer);
    buildings_.select(kPlayer, model::BuildingType::CommandCenter);
    buildings_.place(kPlayer, {0, 0});

    // When the player places a House.
    buildings_.select(kPlayer, model::BuildingType::House);
    buildings_.place(kPlayer, {10, 10});

    // Then its cost is deducted and the population capacity increases.
    EXPECT_EQ(economy.amount(kPlayer, model::Resource::Gold), 80);
    EXPECT_EQ(economy.amount(kPlayer, model::Resource::Wood), 80);
    EXPECT_EQ(economy.populationCapacity(kPlayer), 20);
}
