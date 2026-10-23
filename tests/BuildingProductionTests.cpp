#include "BuildingSystemTestFixture.hpp"

#include "model/build/BuildingComponents.hpp"
#include "model/economy/InventorySystem.hpp"
#include "model/economy/Item.hpp"
#include "model/economy/ProductionComponents.hpp"

using buildingsystem_test::BuildingSystemTest;

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
