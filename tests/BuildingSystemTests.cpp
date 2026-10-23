#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/build/BuildingSystem.hpp"
#include "model/economy/InventoryComponents.hpp"
#include "model/economy/ProductionComponents.hpp"
#include "model/build/BuildingComponents.hpp"
#include "model/components/Owner.hpp"
#include "model/economy/InventorySystem.hpp"
#include "model/economy/Item.hpp"
#include "model/player/PlayerComponents.hpp"
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
    EXPECT_EQ(model::InventorySystem::amount(*playerInventory_, model::ItemId::Iron), 500);
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
    for (const auto item : {model::ItemId::Coal, model::ItemId::Steel,
             model::ItemId::Copper, model::ItemId::Electronics, model::ItemId::Tools}) {
        addResource(item, 100);
    }

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

TEST_F(BuildingSystemTest, TrainingDummyIsAFreeOneByOneBuildingWithCombatHealth)
{
    // Given a player with a Command Center.
    buildings_.select(kPlayer, model::BuildingType::CommandCenter);
    buildings_.place(kPlayer, {0, 0});

    // When the player places a Training Dummy on a free tile.
    const model::GridPos tile{10, 10};
    buildings_.select(kPlayer, model::BuildingType::TrainingDummy);
    buildings_.place(kPlayer, tile);

    // Then it is a free 1x1 construction that units can damage.
    ASSERT_EQ(placements_.size(), 2U);
    const events::BuildingPlaced dummy = placements_.back();
    EXPECT_EQ(dummy.type, model::BuildingType::TrainingDummy);
    EXPECT_EQ(dummy.size, 1);
    EXPECT_TRUE(dummy.origin == tile);
    EXPECT_TRUE(buildings_.isOccupied(tile));
    EXPECT_EQ(model::InventorySystem::amount(*playerInventory_, model::ItemId::Iron), 500);

    const auto* building = world_.tryGet<model::components::Building>(dummy.entity);
    ASSERT_NE(building, nullptr);
    EXPECT_EQ(building->name, "Boneco de Treinamento");
    const auto* health = world_.tryGet<model::components::Health>(dummy.entity);
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
}

TEST_F(BuildingSystemTest, WarehouseCostsOnlyOneHundredIron)
{
    // Given a player with a Command Center and 100 units of each resource.
    buildings_.select(kPlayer, model::BuildingType::CommandCenter);
    buildings_.place(kPlayer, {0, 0});
    addResource(model::ItemId::Coal, 100);
    addResource(model::ItemId::Steel, 100);
    addResource(model::ItemId::Copper, 100);
    addResource(model::ItemId::Electronics, 100);
    addResource(model::ItemId::Tools, 100);

    // When the player builds an Armazem.
    buildings_.select(kPlayer, model::BuildingType::Warehouse);
    buildings_.place(kPlayer, {10, 10});

    // Then only 100 iron are deducted.
    ASSERT_EQ(placements_.size(), 2U);
    EXPECT_EQ(model::InventorySystem::amount(*playerInventory_, model::ItemId::Iron), 400);
    EXPECT_EQ(model::InventorySystem::amount(*playerInventory_, model::ItemId::Coal), 100);
    EXPECT_EQ(model::InventorySystem::amount(*playerInventory_, model::ItemId::Steel), 100);
    EXPECT_EQ(model::InventorySystem::amount(*playerInventory_, model::ItemId::Copper), 100);
    EXPECT_EQ(model::InventorySystem::amount(*playerInventory_, model::ItemId::Electronics), 100);
    EXPECT_EQ(model::InventorySystem::amount(*playerInventory_, model::ItemId::Tools), 100);
}

TEST_F(BuildingSystemTest, IronMineCostsOneHundredIronAndHasInventorySlots)
{
    // Given a player with a completed Command Center and enough iron for a mine.
    buildings_.select(kPlayer, model::BuildingType::CommandCenter);
    buildings_.place(kPlayer, {0, 0});

    // When the player places a mine.
    buildings_.select(kPlayer, model::BuildingType::IronMine);
    buildings_.place(kPlayer, {10, 10});

    // Then the mine is created, its cost is deducted, and it has a 64-slot inventory.
    ASSERT_EQ(placements_.size(), 2U);
    EXPECT_EQ(model::InventorySystem::amount(*playerInventory_, model::ItemId::Iron), 400);
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
    EXPECT_EQ(model::InventorySystem::amount(*playerInventory_, model::ItemId::Iron), 500);

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
    EXPECT_EQ(model::InventorySystem::amount(*playerInventory_, model::ItemId::Iron), 400);
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
    EXPECT_EQ(model::InventorySystem::amount(*playerInventory_, model::ItemId::Iron), 200);
}

TEST_F(BuildingSystemTest, CopperMineProducesOneCopperPerSecond)
{
    // Given a player with a completed Command Center.
    buildings_.select(kPlayer, model::BuildingType::CommandCenter);
    buildings_.place(kPlayer, {0, 0});

    // When the player places a Copper Mine.
    buildings_.select(kPlayer, model::BuildingType::CopperMine);
    buildings_.place(kPlayer, {10, 10});

    // Then it produces one copper per second.
    ASSERT_EQ(placements_.size(), 2U);
    EXPECT_EQ(placements_.back().type, model::BuildingType::CopperMine);
    EXPECT_EQ(placements_.back().size, 2);
    const auto* production = world_.tryGet<model::components::Production>(placements_.back().entity);
    ASSERT_NE(production, nullptr);
    EXPECT_TRUE(production->inputs.empty());
    EXPECT_EQ(production->output, model::ItemId::Copper);
    EXPECT_FLOAT_EQ(production->outputPerSecond, 1.0F);
    EXPECT_EQ(model::InventorySystem::amount(*playerInventory_, model::ItemId::Iron), 400);
}

TEST_F(BuildingSystemTest, ElectronicsFactoryConsumesIronAndCopperAndProducesElectronics)
{
    // Given a player with a completed Command Center.
    buildings_.select(kPlayer, model::BuildingType::CommandCenter);
    buildings_.place(kPlayer, {0, 0});
    addResource(model::ItemId::Steel, 100);

    // When the player places an Electronics Factory.
    buildings_.select(kPlayer, model::BuildingType::ElectronicsFactory);
    buildings_.place(kPlayer, {12, 12});

    // Then it consumes one iron and copper per second and produces half an electronic per second.
    ASSERT_EQ(placements_.size(), 2U);
    EXPECT_EQ(placements_.back().type, model::BuildingType::ElectronicsFactory);
    EXPECT_EQ(placements_.back().size, 3);
    const auto* production = world_.tryGet<model::components::Production>(placements_.back().entity);
    ASSERT_NE(production, nullptr);
    ASSERT_EQ(production->inputs.size(), 2U);
    EXPECT_EQ(production->inputs[0].item, model::ItemId::Iron);
    EXPECT_FLOAT_EQ(production->inputs[0].quantityPerSecond, 1.0F);
    EXPECT_EQ(production->inputs[1].item, model::ItemId::Copper);
    EXPECT_FLOAT_EQ(production->inputs[1].quantityPerSecond, 1.0F);
    EXPECT_EQ(production->output, model::ItemId::Electronics);
    EXPECT_FLOAT_EQ(production->outputPerSecond, 0.5F);
    EXPECT_EQ(model::InventorySystem::amount(*playerInventory_, model::ItemId::Iron), 300);
    EXPECT_EQ(model::InventorySystem::amount(*playerInventory_, model::ItemId::Steel), 0);
}

TEST_F(BuildingSystemTest, ToolFactoryConsumesSteelAndElectronicsAndProducesTools)
{
    // Given a player with a completed Command Center.
    buildings_.select(kPlayer, model::BuildingType::CommandCenter);
    buildings_.place(kPlayer, {0, 0});
    addResource(model::ItemId::Steel, 200);
    addResource(model::ItemId::Electronics, 100);

    // When the player places a Tool Factory.
    buildings_.select(kPlayer, model::BuildingType::ToolFactory);
    buildings_.place(kPlayer, {12, 12});

    // Then it consumes one steel and electronic per second and produces half a tool per second.
    ASSERT_EQ(placements_.size(), 2U);
    EXPECT_EQ(placements_.back().type, model::BuildingType::ToolFactory);
    EXPECT_EQ(placements_.back().size, 3);
    const auto* production = world_.tryGet<model::components::Production>(placements_.back().entity);
    ASSERT_NE(production, nullptr);
    ASSERT_EQ(production->inputs.size(), 2U);
    EXPECT_EQ(production->inputs[0].item, model::ItemId::Steel);
    EXPECT_FLOAT_EQ(production->inputs[0].quantityPerSecond, 1.0F);
    EXPECT_EQ(production->inputs[1].item, model::ItemId::Electronics);
    EXPECT_FLOAT_EQ(production->inputs[1].quantityPerSecond, 1.0F);
    EXPECT_EQ(production->output, model::ItemId::Tools);
    EXPECT_FLOAT_EQ(production->outputPerSecond, 0.5F);
    EXPECT_EQ(model::InventorySystem::amount(*playerInventory_, model::ItemId::Iron), 300);
    EXPECT_EQ(model::InventorySystem::amount(*playerInventory_, model::ItemId::Steel), 0);
    EXPECT_EQ(model::InventorySystem::amount(*playerInventory_, model::ItemId::Electronics), 0);
}

TEST_F(BuildingSystemTest, WeaponFactoryConsumesSteelElectronicsAndToolsAndProducesWeapons)
{
    // Given a player with a completed Command Center and the materials for a weapon factory.
    buildings_.select(kPlayer, model::BuildingType::CommandCenter);
    buildings_.place(kPlayer, {0, 0});
    addResource(model::ItemId::Steel, 200);
    addResource(model::ItemId::Electronics, 100);
    addResource(model::ItemId::Tools, 100);

    // When the player places a Weapon Factory.
    buildings_.select(kPlayer, model::BuildingType::WeaponFactory);
    buildings_.place(kPlayer, {12, 12});

    // Then it consumes one steel, electronic and tool per second and produces half a weapon per second.
    ASSERT_EQ(placements_.size(), 2U);
    EXPECT_EQ(placements_.back().type, model::BuildingType::WeaponFactory);
    EXPECT_EQ(placements_.back().size, 3);
    const auto* production = world_.tryGet<model::components::Production>(placements_.back().entity);
    const auto* building = world_.tryGet<model::components::Building>(placements_.back().entity);
    ASSERT_NE(production, nullptr);
    ASSERT_NE(building, nullptr);
    EXPECT_EQ(building->name, "Fabrica de Armas");
    ASSERT_EQ(production->inputs.size(), 3U);
    EXPECT_EQ(production->inputs[0].item, model::ItemId::Steel);
    EXPECT_FLOAT_EQ(production->inputs[0].quantityPerSecond, 1.0F);
    EXPECT_EQ(production->inputs[1].item, model::ItemId::Electronics);
    EXPECT_FLOAT_EQ(production->inputs[1].quantityPerSecond, 1.0F);
    EXPECT_EQ(production->inputs[2].item, model::ItemId::Tools);
    EXPECT_FLOAT_EQ(production->inputs[2].quantityPerSecond, 1.0F);
    EXPECT_EQ(production->output, model::ItemId::Weapon);
    EXPECT_FLOAT_EQ(production->outputPerSecond, 0.5F);
    EXPECT_EQ(model::InventorySystem::amount(*playerInventory_, model::ItemId::Iron), 300);
    EXPECT_EQ(model::InventorySystem::amount(*playerInventory_, model::ItemId::Steel), 0);
    EXPECT_EQ(model::InventorySystem::amount(*playerInventory_, model::ItemId::Electronics), 0);
    EXPECT_EQ(model::InventorySystem::amount(*playerInventory_, model::ItemId::Tools), 100);
}

TEST_F(BuildingSystemTest, AmmoFactoryConsumesSteelAndCopperAndProducesAmmo)
{
    // Given a player with a completed Command Center and the materials for an ammo factory.
    buildings_.select(kPlayer, model::BuildingType::CommandCenter);
    buildings_.place(kPlayer, {0, 0});
    addResource(model::ItemId::Steel, 200);
    addResource(model::ItemId::Copper, 100);

    // When the player places an Ammo Factory.
    buildings_.select(kPlayer, model::BuildingType::AmmoFactory);
    buildings_.place(kPlayer, {12, 12});

    // Then it consumes one steel and one copper per second and produces half ammo per second.
    ASSERT_EQ(placements_.size(), 2U);
    EXPECT_EQ(placements_.back().type, model::BuildingType::AmmoFactory);
    EXPECT_EQ(placements_.back().size, 3);
    const auto* production = world_.tryGet<model::components::Production>(placements_.back().entity);
    const auto* building = world_.tryGet<model::components::Building>(placements_.back().entity);
    ASSERT_NE(production, nullptr);
    ASSERT_NE(building, nullptr);
    EXPECT_EQ(building->name, "Fabrica de Municao");
    ASSERT_EQ(production->inputs.size(), 2U);
    EXPECT_EQ(production->inputs[0].item, model::ItemId::Steel);
    EXPECT_FLOAT_EQ(production->inputs[0].quantityPerSecond, 1.0F);
    EXPECT_EQ(production->inputs[1].item, model::ItemId::Copper);
    EXPECT_FLOAT_EQ(production->inputs[1].quantityPerSecond, 1.0F);
    EXPECT_EQ(production->output, model::ItemId::Ammo);
    EXPECT_FLOAT_EQ(production->outputPerSecond, 0.5F);
    EXPECT_EQ(model::InventorySystem::amount(*playerInventory_, model::ItemId::Iron), 300);
    EXPECT_EQ(model::InventorySystem::amount(*playerInventory_, model::ItemId::Steel), 0);
    EXPECT_EQ(model::InventorySystem::amount(*playerInventory_, model::ItemId::Copper), 0);
}

TEST_F(BuildingSystemTest, CannotPlaceOrSpendForABuildingWithoutItsMaterials)
{
    // Given a player with a Command Center but no steel or electronics.
    buildings_.select(kPlayer, model::BuildingType::CommandCenter);
    buildings_.place(kPlayer, {0, 0});
    buildings_.select(kPlayer, model::BuildingType::ToolFactory);
    buildings_.moveCursor(kPlayer, model::GridPos{12, 12});

    // When the player tries to place the unaffordable factory.
    buildings_.place(kPlayer, {12, 12});

    // Then placement is rejected, the preview is invalid, and no resources are spent.
    ASSERT_FALSE(previews_.empty());
    EXPECT_FALSE(previews_.back().valid);
    EXPECT_EQ(placements_.size(), 1U);
    EXPECT_EQ(model::InventorySystem::amount(*playerInventory_, model::ItemId::Iron), 500);
}

TEST_F(BuildingSystemTest, CommonBuildingsCostOneHundredOfEveryItem)
{
    // Given a player with a Command Center and 100 units of every resource.
    buildings_.select(kPlayer, model::BuildingType::CommandCenter);
    buildings_.place(kPlayer, {0, 0});
    for (const auto item : {model::ItemId::Coal, model::ItemId::Steel,
             model::ItemId::Copper, model::ItemId::Electronics, model::ItemId::Tools}) {
        addResource(item, 100);
    }

    // When the player builds a wall.
    buildings_.select(kPlayer, model::BuildingType::Wall);
    buildings_.place(kPlayer, {10, 10});

    // Then 100 units of each resource are deducted.
    ASSERT_EQ(placements_.size(), 2U);
    for (const auto item : {model::ItemId::Iron, model::ItemId::Coal, model::ItemId::Steel,
             model::ItemId::Copper, model::ItemId::Electronics, model::ItemId::Tools}) {
        EXPECT_EQ(model::InventorySystem::amount(*playerInventory_, item),
            item == model::ItemId::Iron ? 400 : 0);
    }
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
    EXPECT_EQ(wallEntry->cost, "Gratis");

    // Then the player can build without materials and the inventory remains unchanged.
    buildings_.select(kPlayer, model::BuildingType::CommandCenter);
    buildings_.place(kPlayer, {0, 0});
    buildings_.select(kPlayer, model::BuildingType::Wall);
    buildings_.moveCursor(kPlayer, model::GridPos{10, 10});
    ASSERT_FALSE(previews_.empty());
    EXPECT_TRUE(previews_.back().valid);
    buildings_.place(kPlayer, {10, 10});
    ASSERT_EQ(placements_.size(), 2U);
    for (const auto item : {model::ItemId::Iron, model::ItemId::Coal, model::ItemId::Steel,
             model::ItemId::Copper, model::ItemId::Electronics, model::ItemId::Tools}) {
        EXPECT_EQ(model::InventorySystem::amount(*playerInventory_, item), 0);
    }
}
