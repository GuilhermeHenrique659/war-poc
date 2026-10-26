#include "BuildingSystemTestFixture.hpp"

#include "core/Events.hpp"
#include "model/build/BuildingComponents.hpp"
#include "model/build/ConstructionComponents.hpp"
#include "model/economy/InventoryComponents.hpp"
#include "model/economy/InventorySystem.hpp"
#include "model/economy/Item.hpp"
#include "model/economy/Market.hpp"
#include "model/economy/ProductionComponents.hpp"

#include <algorithm>

using buildingsystem_test::BuildingSystemTest;

namespace {

[[nodiscard]] const events::BuildBarEntry* barEntry(
    const events::BuildBarChanged& bar, model::BuildingType type)
{
    for (const events::BuildBarEntry& entry : bar.entries) {
        if (entry.type == type) {
            return &entry;
        }
    }
    return nullptr;
}

} // namespace

TEST_F(BuildingSystemTest, RequiresACommandCenterBeforeWallsCanBeBuilt)
{
    // Given a player who has not placed a Command Center.
    buildings_.select(kPlayer, model::BuildingType::Wall);

    // When the player attempts to place a wall.
    buildings_.place(kPlayer, {0, 0});

    // Then no wall is created.
    EXPECT_TRUE(placements_.empty());
    EXPECT_TRUE(sites_.empty());
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

TEST_F(BuildingSystemTest, MovesStartingMaterialsIntoTheCommandCenter)
{
    // Given a player who also carries combat supplies.
    addResource(model::ItemId::Weapon, 2);
    addResource(model::ItemId::Ammo, 50);
    buildings_.select(kPlayer, model::BuildingType::CommandCenter);

    // When the Command Center is placed.
    buildings_.place(kPlayer, {0, 0});

    // Then the construction materials move to it and the combat supplies stay with the player.
    ASSERT_EQ(placements_.size(), 1U);
    auto* center = buildingInventory(placements_.front().entity);
    ASSERT_NE(center, nullptr);
    EXPECT_EQ(model::InventorySystem::amount(*center, model::ItemId::Iron), 500);
    EXPECT_EQ(model::InventorySystem::amount(*playerInventory_, model::ItemId::Iron), 0);
    EXPECT_EQ(model::InventorySystem::amount(*playerInventory_, model::ItemId::Weapon), 2);
    EXPECT_EQ(model::InventorySystem::amount(*playerInventory_, model::ItemId::Ammo), 50);
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
    EXPECT_TRUE(sites_.empty());
}

TEST_F(BuildingSystemTest, UnlocksWallsAfterTheCommandCenterIsBuilt)
{
    // Given a player who has already placed their Command Center.
    buildings_.select(kPlayer, model::BuildingType::CommandCenter);
    buildings_.place(kPlayer, {0, 0});

    // When the player selects and places a wall on a free tile.
    const model::GridPos wallTile{10, 10};
    const auto wall = placeReady(model::BuildingType::Wall, wallTile);

    // Then the wall is created and reserves its tile.
    ASSERT_NE(wall, 0U);
    ASSERT_EQ(placements_.size(), 2U);
    EXPECT_EQ(placements_.back().type, model::BuildingType::Wall);
    EXPECT_EQ(placements_.back().size, 1);
    EXPECT_TRUE(placements_.back().origin == wallTile);
    EXPECT_TRUE(buildings_.isOccupied(wallTile));
}

TEST_F(BuildingSystemTest, TrainingDummyIsAFreeOneByOneBuildingWithCombatHealth)
{
    // Given a player with a Command Center.
    buildings_.select(kPlayer, model::BuildingType::CommandCenter);
    buildings_.place(kPlayer, {0, 0});

    // When the player places a Training Dummy on a free tile.
    const model::GridPos tile{10, 10};
    const auto dummy = placeReady(model::BuildingType::TrainingDummy, tile);

    // Then it is a free 1x1 construction that units can damage.
    ASSERT_NE(dummy, 0U);
    ASSERT_EQ(placements_.size(), 2U);
    EXPECT_EQ(placements_.back().type, model::BuildingType::TrainingDummy);
    EXPECT_EQ(placements_.back().size, 1);
    EXPECT_TRUE(placements_.back().origin == tile);
    EXPECT_TRUE(buildings_.isOccupied(tile));

    const auto* building = world_.tryGet<model::components::Building>(dummy);
    ASSERT_NE(building, nullptr);
    EXPECT_EQ(building->name, "Boneco de Treinamento");
    const auto* health = world_.tryGet<model::components::Health>(dummy);
    ASSERT_NE(health, nullptr);
    EXPECT_EQ(health->current, 100);
    EXPECT_EQ(health->max, 100);
}

TEST_F(BuildingSystemTest, TrainingDummyRequiresACommandCenter)
{
    // Given a player who has not placed a Command Center.
    buildings_.select(kPlayer, model::BuildingType::TrainingDummy);

    // When the player attempts to place a Training Dummy.
    buildings_.place(kPlayer, {0, 0});

    // Then nothing is built.
    EXPECT_TRUE(placements_.empty());
    EXPECT_TRUE(sites_.empty());
}

TEST_F(BuildingSystemTest, PlacingATrainingDummyLeavesBuildMode)
{
    // Given a player with a Command Center and the dummy selected.
    buildings_.select(kPlayer, model::BuildingType::CommandCenter);
    buildings_.place(kPlayer, {0, 0});
    buildings_.select(kPlayer, model::BuildingType::TrainingDummy);
    ASSERT_FALSE(buildBars_.empty());
    ASSERT_TRUE(buildBars_.back().selected.has_value());

    // When the dummy is placed.
    buildings_.place(kPlayer, {10, 10});

    // Then build mode ends, so troops can be selected right away.
    ASSERT_FALSE(buildBars_.empty());
    EXPECT_FALSE(buildBars_.back().selected.has_value());
}

TEST_F(BuildingSystemTest, WarehouseCostsOnlyOneHundredIron)
{
    // Given the building cost table.
    const model::BuildingCosts& costs = model::buildingCosts(model::BuildingType::Warehouse);

    // Then the warehouse needs only 100 iron.
    int iron = 0;
    int others = 0;
    for (const model::BuildingCost& cost : costs) {
        if (cost.item == model::ItemId::Iron) {
            iron += cost.quantity;
        } else {
            others += cost.quantity;
        }
    }
    EXPECT_EQ(iron, 100);
    EXPECT_EQ(others, 0);
}

TEST_F(BuildingSystemTest, IronMineCostsOneHundredIronAndHasInventorySlots)
{
    // Given a player with a completed Command Center.
    buildings_.select(kPlayer, model::BuildingType::CommandCenter);
    buildings_.place(kPlayer, {0, 0});

    // When the player places and finishes a mine.
    const auto mine = placeReady(model::BuildingType::IronMine, {10, 10});

    // Then the mine is created and has a 64-slot inventory.
    ASSERT_NE(mine, 0U);
    ASSERT_EQ(placements_.size(), 2U);
    const auto* inventory = world_.tryGet<model::components::Inventory>(mine);
    const auto* production = world_.tryGet<model::components::Production>(mine);
    const auto* building = world_.tryGet<model::components::Building>(mine);
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

TEST_F(BuildingSystemTest, StartsConstructionWithoutMaterialsAndSpendsNothing)
{
    // Given a player with a Command Center and no materials for a factory.
    buildings_.select(kPlayer, model::BuildingType::CommandCenter);
    buildings_.place(kPlayer, {0, 0});
    buildings_.select(kPlayer, model::BuildingType::ToolFactory);
    buildings_.moveCursor(kPlayer, model::GridPos{12, 12});
    ASSERT_FALSE(previews_.empty());
    EXPECT_TRUE(previews_.back().valid);

    // When the player places the unaffordable factory.
    buildings_.place(kPlayer, {12, 12});

    // Then a construction site is created and nothing is spent.
    ASSERT_EQ(sites_.size(), 1U);
    EXPECT_EQ(sites_.back().type, model::BuildingType::ToolFactory);
    EXPECT_EQ(placements_.size(), 1U);
    EXPECT_EQ(model::InventorySystem::amount(*playerInventory_, model::ItemId::Iron), 0);
}

TEST_F(BuildingSystemTest, CommonBuildingsCostOneHundredOfEveryItem)
{
    // Given the building cost table.
    const model::BuildingCosts& costs = model::buildingCosts(model::BuildingType::Wall);

    // Then a common building needs 100 units of each construction material.
    for (const model::ItemId item : model::kConstructionMaterials) {
        int quantity = 0;
        for (const model::BuildingCost& cost : costs) {
            if (cost.item == item) {
                quantity += cost.quantity;
            }
        }
        EXPECT_EQ(quantity, 100) << "item " << static_cast<int>(item);
    }
}

TEST_F(BuildingSystemTest, BuildBarPublishesSemanticLockReasonsAndCosts)
{
    // Given a player who has not placed a Command Center yet.
    buildings_.initializePlayer(kPlayer);

    // Then the bar exposes why each entry is locked instead of a ready-made sentence.
    const auto wallEntry = std::find_if(buildBars_.back().entries.begin(), buildBars_.back().entries.end(),
        [](const events::BuildBarEntry& entry) { return entry.type == model::BuildingType::Wall; });
    ASSERT_NE(wallEntry, buildBars_.back().entries.end());
    EXPECT_FALSE(wallEntry->enabled);
    EXPECT_EQ(wallEntry->lockReason, events::BuildLockReason::RequiresCommandCenter);
    EXPECT_FALSE(wallEntry->free);

    // And the material list is data, not text.
    ASSERT_EQ(wallEntry->costs.size(), 6U);
    EXPECT_EQ(wallEntry->costs[0].item, model::ItemId::Iron);
    EXPECT_EQ(wallEntry->costs[0].quantity, 100);

    const auto commandCenterEntry = std::find_if(buildBars_.back().entries.begin(), buildBars_.back().entries.end(),
        [](const events::BuildBarEntry& entry) { return entry.type == model::BuildingType::CommandCenter; });
    ASSERT_NE(commandCenterEntry, buildBars_.back().entries.end());
    EXPECT_TRUE(commandCenterEntry->enabled);
    EXPECT_EQ(commandCenterEntry->lockReason, events::BuildLockReason::None);
    EXPECT_TRUE(commandCenterEntry->costs.empty());
}

TEST_F(BuildingSystemTest, BuildBarReportsAnAlreadyBuiltCommandCenter)
{
    // Given a player who already owns a Command Center.
    buildings_.initializePlayer(kPlayer);
    buildings_.select(kPlayer, model::BuildingType::CommandCenter);
    buildings_.place(kPlayer, {0, 0});

    // Then the Command Center entry is locked with the semantic reason.
    const auto entry = std::find_if(buildBars_.back().entries.begin(), buildBars_.back().entries.end(),
        [](const events::BuildBarEntry& barEntry) {
            return barEntry.type == model::BuildingType::CommandCenter;
        });
    ASSERT_NE(entry, buildBars_.back().entries.end());
    EXPECT_FALSE(entry->enabled);
    EXPECT_EQ(entry->lockReason, events::BuildLockReason::AlreadyBuilt);
}

TEST_F(BuildingSystemTest, DebugModeMakesConstructionFree)
{
    // Given a player with no resources and an initialized build bar.
    for (const auto item : {model::ItemId::Iron, model::ItemId::Coal, model::ItemId::Steel,
             model::ItemId::Copper, model::ItemId::Electronics, model::ItemId::Tools}) {
        const int amount = model::InventorySystem::amount(*playerInventory_, item);
        EXPECT_EQ(model::InventorySystem::remove(*playerInventory_, item, amount), amount);
    }
    buildings_.initializePlayer(kPlayer);

    // When debug mode is activated.
    bus_.publish(events::DebugToggleRequested{});

    const auto wallEntry = std::find_if(buildBars_.back().entries.begin(), buildBars_.back().entries.end(),
        [](const events::BuildBarEntry& entry) { return entry.type == model::BuildingType::Wall; });
    ASSERT_NE(wallEntry, buildBars_.back().entries.end());
    EXPECT_TRUE(wallEntry->free);

    // Then the player can start construction without materials and nothing is deducted.
    buildings_.select(kPlayer, model::BuildingType::CommandCenter);
    buildings_.place(kPlayer, {0, 0});
    buildings_.select(kPlayer, model::BuildingType::Wall);
    buildings_.moveCursor(kPlayer, model::GridPos{10, 10});
    ASSERT_FALSE(previews_.empty());
    EXPECT_TRUE(previews_.back().valid);
    buildings_.place(kPlayer, {10, 10});
    ASSERT_EQ(sites_.size(), 1U);
    for (const auto item : {model::ItemId::Iron, model::ItemId::Coal, model::ItemId::Steel,
             model::ItemId::Copper, model::ItemId::Electronics, model::ItemId::Tools}) {
        EXPECT_EQ(model::InventorySystem::amount(*playerInventory_, item), 0);
    }
}

TEST_F(BuildingSystemTest, BuildBarShowsTheCreditCostAtTheCurrentPrices)
{
    // Given a player whose build bar was published at the base prices.
    buildings_.initializePlayer(kPlayer);
    buildings_.select(kPlayer, model::BuildingType::Wall);
    ASSERT_FALSE(buildBars_.empty());

    // Then a wall costs 100 of each common material at its base price.
    const events::BuildBarEntry* wall = barEntry(buildBars_.back(), model::BuildingType::Wall);
    ASSERT_NE(wall, nullptr);
    EXPECT_DOUBLE_EQ(wall->creditCost, 100.0 * (1.0 + 1.0 + 5.0 + 1.0 + 5.0 + 25.0));
}

TEST_F(BuildingSystemTest, BuildBarRefreshesTheCreditCostWhenPricesChange)
{
    // Given a player whose build bar was published at the base prices.
    buildings_.initializePlayer(kPlayer);
    buildings_.select(kPlayer, model::BuildingType::Wall);
    ASSERT_FALSE(buildBars_.empty());

    // When the market doubles the iron price.
    events::MarketPricesChanged prices{};
    for (const model::ItemId item : model::kAllItems) {
        prices.prices[model::itemIndex(item)] = model::marketDefinition(item).basePrice;
    }
    prices.prices[model::itemIndex(model::ItemId::Iron)] = 2.0;
    bus_.publish(prices);

    // Then the wall estimate follows the new price.
    const events::BuildBarEntry* wall = barEntry(buildBars_.back(), model::BuildingType::Wall);
    ASSERT_NE(wall, nullptr);
    EXPECT_DOUBLE_EQ(wall->creditCost, 100.0 * (2.0 + 1.0 + 5.0 + 1.0 + 5.0 + 25.0));
}
