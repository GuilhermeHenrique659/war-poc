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

    // When the player places and finishes a Coal Mine.
    const auto mine = placeReady(model::BuildingType::CoalMine, {10, 10});

    // Then it has a primary recipe for one coal per second.
    ASSERT_NE(mine, 0U);
    ASSERT_EQ(placements_.size(), 2U);
    const auto* production = world_.tryGet<model::components::Production>(mine);
    ASSERT_NE(production, nullptr);
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
    const auto steelworks = placeReady(model::BuildingType::Steelworks, {12, 12});

    // Then it has the defined input and output rates and a 3x3 footprint.
    ASSERT_NE(steelworks, 0U);
    ASSERT_EQ(placements_.size(), 2U);
    EXPECT_EQ(placements_.back().type, model::BuildingType::Steelworks);
    EXPECT_EQ(placements_.back().size, 3);
    const auto* production = world_.tryGet<model::components::Production>(steelworks);
    ASSERT_NE(production, nullptr);
    ASSERT_EQ(production->inputs.size(), 2U);
    EXPECT_EQ(production->inputs[0].item, model::ItemId::Iron);
    EXPECT_FLOAT_EQ(production->inputs[0].quantityPerSecond, 1.0F);
    EXPECT_EQ(production->inputs[1].item, model::ItemId::Coal);
    EXPECT_FLOAT_EQ(production->inputs[1].quantityPerSecond, 1.0F);
    EXPECT_EQ(production->output, model::ItemId::Steel);
    EXPECT_FLOAT_EQ(production->outputPerSecond, 0.5F);
}

TEST_F(BuildingSystemTest, CopperMineProducesOneCopperPerSecond)
{
    // Given a player with a completed Command Center.
    buildings_.select(kPlayer, model::BuildingType::CommandCenter);
    buildings_.place(kPlayer, {0, 0});

    // When the player places and finishes a Copper Mine.
    const auto mine = placeReady(model::BuildingType::CopperMine, {10, 10});

    // Then it produces one copper per second.
    ASSERT_NE(mine, 0U);
    ASSERT_EQ(placements_.size(), 2U);
    EXPECT_EQ(placements_.back().type, model::BuildingType::CopperMine);
    EXPECT_EQ(placements_.back().size, 2);
    const auto* production = world_.tryGet<model::components::Production>(mine);
    ASSERT_NE(production, nullptr);
    EXPECT_TRUE(production->inputs.empty());
    EXPECT_EQ(production->output, model::ItemId::Copper);
    EXPECT_FLOAT_EQ(production->outputPerSecond, 1.0F);
}

TEST_F(BuildingSystemTest, ElectronicsFactoryConsumesIronAndCopperAndProducesElectronics)
{
    // Given a player with a completed Command Center.
    buildings_.select(kPlayer, model::BuildingType::CommandCenter);
    buildings_.place(kPlayer, {0, 0});

    // When the player places and finishes an Electronics Factory.
    const auto factory = placeReady(model::BuildingType::ElectronicsFactory, {12, 12});

    // Then it consumes one iron and copper per second and produces half an electronic per second.
    ASSERT_NE(factory, 0U);
    ASSERT_EQ(placements_.size(), 2U);
    EXPECT_EQ(placements_.back().type, model::BuildingType::ElectronicsFactory);
    EXPECT_EQ(placements_.back().size, 3);
    const auto* production = world_.tryGet<model::components::Production>(factory);
    ASSERT_NE(production, nullptr);
    ASSERT_EQ(production->inputs.size(), 2U);
    EXPECT_EQ(production->inputs[0].item, model::ItemId::Iron);
    EXPECT_FLOAT_EQ(production->inputs[0].quantityPerSecond, 1.0F);
    EXPECT_EQ(production->inputs[1].item, model::ItemId::Copper);
    EXPECT_FLOAT_EQ(production->inputs[1].quantityPerSecond, 1.0F);
    EXPECT_EQ(production->output, model::ItemId::Electronics);
    EXPECT_FLOAT_EQ(production->outputPerSecond, 0.5F);
}

TEST_F(BuildingSystemTest, ToolFactoryConsumesSteelAndElectronicsAndProducesTools)
{
    // Given a player with a completed Command Center.
    buildings_.select(kPlayer, model::BuildingType::CommandCenter);
    buildings_.place(kPlayer, {0, 0});

    // When the player places and finishes a Tool Factory.
    const auto factory = placeReady(model::BuildingType::ToolFactory, {12, 12});

    // Then it consumes one steel and electronic per second and produces half a tool per second.
    ASSERT_NE(factory, 0U);
    ASSERT_EQ(placements_.size(), 2U);
    EXPECT_EQ(placements_.back().type, model::BuildingType::ToolFactory);
    EXPECT_EQ(placements_.back().size, 3);
    const auto* production = world_.tryGet<model::components::Production>(factory);
    ASSERT_NE(production, nullptr);
    ASSERT_EQ(production->inputs.size(), 2U);
    EXPECT_EQ(production->inputs[0].item, model::ItemId::Steel);
    EXPECT_FLOAT_EQ(production->inputs[0].quantityPerSecond, 1.0F);
    EXPECT_EQ(production->inputs[1].item, model::ItemId::Electronics);
    EXPECT_FLOAT_EQ(production->inputs[1].quantityPerSecond, 1.0F);
    EXPECT_EQ(production->output, model::ItemId::Tools);
    EXPECT_FLOAT_EQ(production->outputPerSecond, 0.5F);
}

TEST_F(BuildingSystemTest, WeaponFactoryConsumesSteelElectronicsAndToolsAndProducesWeapons)
{
    // Given a player with a completed Command Center.
    buildings_.select(kPlayer, model::BuildingType::CommandCenter);
    buildings_.place(kPlayer, {0, 0});

    // When the player places and finishes a Weapon Factory.
    const auto factory = placeReady(model::BuildingType::WeaponFactory, {12, 12});

    // Then it consumes one steel, electronic and tool per second and produces half a weapon per second.
    ASSERT_NE(factory, 0U);
    ASSERT_EQ(placements_.size(), 2U);
    EXPECT_EQ(placements_.back().type, model::BuildingType::WeaponFactory);
    EXPECT_EQ(placements_.back().size, 3);
    const auto* production = world_.tryGet<model::components::Production>(factory);
    const auto* building = world_.tryGet<model::components::Building>(factory);
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
}

TEST_F(BuildingSystemTest, AmmoFactoryConsumesSteelAndCopperAndProducesAmmo)
{
    // Given a player with a completed Command Center.
    buildings_.select(kPlayer, model::BuildingType::CommandCenter);
    buildings_.place(kPlayer, {0, 0});

    // When the player places and finishes an Ammo Factory.
    const auto factory = placeReady(model::BuildingType::AmmoFactory, {12, 12});

    // Then it consumes one steel and one copper per second and produces half ammo per second.
    ASSERT_NE(factory, 0U);
    ASSERT_EQ(placements_.size(), 2U);
    EXPECT_EQ(placements_.back().type, model::BuildingType::AmmoFactory);
    EXPECT_EQ(placements_.back().size, 3);
    const auto* production = world_.tryGet<model::components::Production>(factory);
    const auto* building = world_.tryGet<model::components::Building>(factory);
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
}
