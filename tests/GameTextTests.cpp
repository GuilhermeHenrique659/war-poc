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
    EXPECT_EQ(view::ui::buildingName(model::BuildingType::TradeCenter), "Centro Comercial");
    EXPECT_EQ(view::ui::buildingName(model::BuildingType::TrainingDummy), "Boneco de Treinamento");
}

TEST_F(GameTextTest, TranslatesUnitAndItemNames)
{
    EXPECT_EQ(view::ui::unitName(model::UnitType::Soldier), "soldado");
    EXPECT_EQ(view::ui::unitName(model::UnitType::Engineer), "engenheiro");
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

TEST_F(GameTextTest, FormatsTheCalendarDateLine)
{
    // Given month 2, week 3.
    events::TimeChanged time{};
    time.month = 2;
    time.week = 3;
    time.day = 4;
    time.tickInDay = 270;

    // Then the date line names the month and week.
    EXPECT_EQ(view::ui::dateText(time), "Mês 2 · Semana 3");
}

TEST_F(GameTextTest, FormatsTheElapsedTimeInsideTheDay)
{
    // Given 270 ticks into the fourth day.
    events::TimeChanged time{};
    time.month = 1;
    time.week = 1;
    time.day = 4;
    time.tickInDay = 270;

    // Then the clock line shows 4 minutes and 30 seconds.
    EXPECT_EQ(view::ui::dayClockText(time), "Dia 4 · 04:30");
}

TEST_F(GameTextTest, FormatsTheLastSecondOfTheDay)
{
    // Given the final tick of the first day.
    events::TimeChanged time{};
    time.day = 1;
    time.tickInDay = 599;

    // Then the clock stops just before ten minutes.
    EXPECT_EQ(view::ui::dayClockText(time), "Dia 1 · 09:59");
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

TEST_F(GameTextTest, FormatsTheCreditBalanceWithoutDecimals)
{
    EXPECT_EQ(view::ui::creditLabel(1000.0), "Crédito: 1000");
    EXPECT_EQ(view::ui::creditLabel(-120.0), "Crédito: -120");
}

TEST_F(GameTextTest, FormatsTheWeeklyInterestWithoutDecimals)
{
    EXPECT_EQ(view::ui::weeklyInterestLabel(0.0), "Juros/semana: 0");
    EXPECT_EQ(view::ui::weeklyInterestLabel(6.0), "Juros/semana: 6");
}

TEST_F(GameTextTest, FormatsTheEstimatedCreditOfABuild)
{
    EXPECT_EQ(view::ui::buildCreditLabel(3800.0), "EC 3800");
}

TEST_F(GameTextTest, FormatsATradePanelRow)
{
    // Given an iron row with part of the weekly quota already sold.
    events::TradeRow row{};
    row.item = model::ItemId::Iron;
    row.price = 1.5;
    row.remainingQuota = 700;
    row.weeklyQuota = 1000;
    row.stock = 250;

    // Then the row names the item, the price, the quota and the stock.
    EXPECT_EQ(view::ui::tradeRowText(row), "Ferro  preço 1.5  cota 700/1000  estoque 250");
}
