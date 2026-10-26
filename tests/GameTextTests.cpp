#include "view/ui/GameText.hpp"
#include "view/ui/TextCatalog.hpp"

#include <gtest/gtest.h>

#include <vector>

namespace {

class GameTextTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        view::ui::TextCatalog::instance().load("pt_br");
    }
};

} // namespace

TEST_F(GameTextTest, TranslatesBuildingNames)
{
    EXPECT_EQ(view::ui::buildingName(model::BuildingType::CommandCenter), "Centro de Comando");
    EXPECT_EQ(view::ui::buildingName(model::BuildingType::Wall), "Muro");
    EXPECT_EQ(view::ui::buildingName(model::BuildingType::TrainingDummy), "Boneco de Treinamento");
}

TEST_F(GameTextTest, TranslatesUnitAndItemNames)
{
    EXPECT_EQ(view::ui::unitName(model::UnitType::Soldier), "Soldado");
    EXPECT_EQ(view::ui::unitName(model::UnitType::Goat), "Bode");
    EXPECT_EQ(view::ui::unitName(model::UnitType::Combined), "Combinado");
    EXPECT_EQ(view::ui::itemName(model::ItemId::Iron), "Ferro");
    EXPECT_EQ(view::ui::itemName(model::ItemId::Ammo), "Municao");
}

TEST_F(GameTextTest, UsesCompactItemNamesForCosts)
{
    EXPECT_EQ(view::ui::shortItemName(model::ItemId::Iron), "Fe");
    EXPECT_EQ(view::ui::shortItemName(model::ItemId::Electronics), "Ele");
    EXPECT_EQ(view::ui::shortItemName(model::ItemId::Weapon), "Arma");
}

TEST_F(GameTextTest, TranslatesMenuAndSceneNames)
{
    EXPECT_EQ(view::ui::menuLabel(model::MenuAction::StartGame), "Iniciar");
    EXPECT_EQ(view::ui::menuLabel(model::MenuAction::Settings), "Configurações");
    EXPECT_EQ(view::ui::menuLabel(model::MenuAction::Quit), "Sair");
    EXPECT_EQ(view::ui::sceneName(model::Scene::Playing), "Partida");
}

TEST_F(GameTextTest, TranslatesBuildLockReasons)
{
    EXPECT_EQ(view::ui::lockReason(events::BuildLockReason::None), "");
    EXPECT_EQ(view::ui::lockReason(events::BuildLockReason::AlreadyBuilt), "Ja construido");
    EXPECT_EQ(view::ui::lockReason(events::BuildLockReason::RequiresCommandCenter), "Requer Centro");
}

TEST_F(GameTextTest, FreeCostsUseTheFreeLabel)
{
    EXPECT_EQ(view::ui::costText({}, true), "Gratis");
}

TEST_F(GameTextTest, EmptyCostListAlsoReadsAsFree)
{
    EXPECT_EQ(view::ui::costText({}, false), "Gratis");
}

TEST_F(GameTextTest, FormatsEachMaterialWithItsShortName)
{
    const std::vector<events::CostEntry> costs{{model::ItemId::Iron, 100}, {model::ItemId::Coal, 50}};

    EXPECT_EQ(view::ui::costText(costs, false), "Fe 100 Carv 50");
}

TEST_F(GameTextTest, CollapsesTheCommonCostIntoOneLine)
{
    const std::vector<events::CostEntry> costs{{model::ItemId::Iron, 100}, {model::ItemId::Coal, 100},
        {model::ItemId::Steel, 100}, {model::ItemId::Copper, 100}, {model::ItemId::Electronics, 100},
        {model::ItemId::Tools, 100}};

    EXPECT_EQ(view::ui::costText(costs, false), "100 cada recurso");
}
