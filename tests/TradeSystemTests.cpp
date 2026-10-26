#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/build/BuildingComponents.hpp"
#include "model/build/BuildingSystem.hpp"
#include "model/components/Owner.hpp"
#include "model/economy/EconomySystem.hpp"
#include "model/economy/InventoryComponents.hpp"
#include "model/economy/InventorySystem.hpp"
#include "model/economy/Item.hpp"
#include "model/economy/Market.hpp"
#include "model/economy/TradeComponents.hpp"
#include "model/economy/TradeSystem.hpp"
#include "model/player/PlayerComponents.hpp"
#include "ConstructionTestHelper.hpp"

#include <gtest/gtest.h>

#include <vector>

namespace {

constexpr model::PlayerId kPlayer = 1;

class TradeSystemTests : public ::testing::Test {
protected:
    TradeSystemTests()
        : buildings_(bus_, world_)
        , economy_(bus_)
        , trade_(bus_, world_, economy_)
    {
        bus_.subscribe<events::TradePanelChanged>(
            [this](const events::TradePanelChanged& event) { panels_.push_back(event); });

        const auto player = world_.create();
        world_.emplace<model::components::PlayerProfile>(player, kPlayer, "Tester");
        world_.emplace<model::components::Inventory>(player, std::size_t{64});
    }

    [[nodiscard]] model::ecs::Entity makeCenter(
        model::PlayerId owner = kPlayer, std::size_t slots = 64)
    {
        const auto center = world_.create();
        world_.emplace<model::components::Building>(center, model::BuildingType::TradeCenter);
        world_.emplace<model::components::Owner>(center, owner);
        world_.emplace<model::components::Inventory>(center, slots);
        world_.emplace<model::components::TradeQuota>(center);
        return center;
    }

    void stock(model::ecs::Entity entity, model::ItemId item, int quantity)
    {
        auto* inventory = world_.tryGet<model::components::Inventory>(entity);
        ASSERT_NE(inventory, nullptr);
        EXPECT_EQ(model::InventorySystem::add(*inventory, item, quantity), quantity);
    }

    [[nodiscard]] int amountIn(model::ecs::Entity entity, model::ItemId item)
    {
        const auto* inventory = world_.tryGet<model::components::Inventory>(entity);
        return inventory == nullptr ? -1 : model::InventorySystem::amount(*inventory, item);
    }

    [[nodiscard]] model::ecs::Entity place(model::BuildingType type, model::GridPos anchor)
    {
        return test_support::placeAndComplete(buildings_, bus_, world_, kPlayer, type, anchor);
    }

    [[nodiscard]] const events::TradeRow* rowFor(const events::TradePanelChanged& panel, model::ItemId item)
    {
        for (const events::TradeRow& row : panel.rows) {
            if (row.item == item) {
                return &row;
            }
        }
        return nullptr;
    }

    core::EventBus bus_;
    model::ecs::Registry world_;
    model::BuildingSystem buildings_;
    model::EconomySystem economy_;
    model::TradeSystem trade_;
    std::vector<events::TradePanelChanged> panels_;
};

} // namespace

TEST_F(TradeSystemTests, SellsStoredItemsUpToTheWeeklyQuota)
{
    // Given a Trade Center storing more iron than its weekly quota.
    const auto center = makeCenter();
    stock(center, model::ItemId::Iron, 1500);
    economy_.initializePlayer(kPlayer);

    // When the trade system runs.
    trade_.update();

    // Then it sells exactly the quota at the current price.
    EXPECT_EQ(amountIn(center, model::ItemId::Iron), 500);
    EXPECT_DOUBLE_EQ(economy_.balance(kPlayer), model::kStartingCredit + 1000.0);
    EXPECT_EQ(economy_.market().flows(model::ItemId::Iron).sold, 1000);
}

TEST_F(TradeSystemTests, RenewsTheQuotaWhenANewWeekBegins)
{
    // Given a Trade Center that already sold this week's iron quota.
    const auto center = makeCenter();
    stock(center, model::ItemId::Iron, 1500);
    economy_.initializePlayer(kPlayer);
    trade_.update();
    ASSERT_EQ(amountIn(center, model::ItemId::Iron), 500);

    // When a new week begins and the trade system runs again.
    bus_.publish(events::WeekStarted{2});
    trade_.update();

    // Then the remaining stock is sold at the new market price.
    EXPECT_EQ(amountIn(center, model::ItemId::Iron), 0);
    EXPECT_DOUBLE_EQ(economy_.balance(kPlayer), model::kStartingCredit + 1000.0 + 1000.0);
}

TEST_F(TradeSystemTests, SellsWeaponsAndAmmoUpToTheirLargeQuotas)
{
    // Given a Trade Center with weapons and ammo.
    const auto center = makeCenter(kPlayer, 700);
    stock(center, model::ItemId::Weapon, 600);
    stock(center, model::ItemId::Ammo, 3500);
    economy_.initializePlayer(kPlayer);

    // When the trade system runs.
    trade_.update();

    // Then the surplus above each quota stays stored.
    EXPECT_EQ(amountIn(center, model::ItemId::Weapon), 100);
    EXPECT_EQ(amountIn(center, model::ItemId::Ammo), 500);
    EXPECT_DOUBLE_EQ(economy_.balance(kPlayer),
        model::kStartingCredit + 500.0 * 80.0 + 3000.0 * 2.0);
}

TEST_F(TradeSystemTests, DoesNotSellForPlayersWithoutAnAccount)
{
    // Given a Trade Center owned by a player that never joined.
    const auto center = makeCenter(model::PlayerId{99});
    stock(center, model::ItemId::Iron, 100);

    // When the trade system runs.
    trade_.update();

    // Then nothing leaves the building.
    EXPECT_EQ(amountIn(center, model::ItemId::Iron), 100);
}

TEST_F(TradeSystemTests, PanelReportsPriceRemainingQuotaAndStock)
{
    // Given a Trade Center with iron stock and part of the quota already sold.
    const auto center = makeCenter();
    stock(center, model::ItemId::Iron, 250);
    auto* quota = world_.tryGet<model::components::TradeQuota>(center);
    ASSERT_NE(quota, nullptr);
    quota->soldThisWeek[model::itemIndex(model::ItemId::Iron)] = 300;

    // When the panel is published.
    trade_.publishPanel(kPlayer, center);

    // Then every item is listed and iron shows its live numbers.
    ASSERT_EQ(panels_.size(), 1U);
    EXPECT_TRUE(panels_[0].open);
    EXPECT_EQ(panels_[0].player, kPlayer);
    EXPECT_EQ(panels_[0].building, center);
    ASSERT_EQ(panels_[0].rows.size(), static_cast<std::size_t>(model::ItemId::Count));
    const events::TradeRow* iron = rowFor(panels_[0], model::ItemId::Iron);
    ASSERT_NE(iron, nullptr);
    EXPECT_DOUBLE_EQ(iron->price, 1.0);
    EXPECT_EQ(iron->remainingQuota, 700);
    EXPECT_EQ(iron->weeklyQuota, 1000);
    EXPECT_EQ(iron->stock, 250);
    const events::TradeRow* weapons = rowFor(panels_[0], model::ItemId::Weapon);
    ASSERT_NE(weapons, nullptr);
    EXPECT_EQ(weapons->weeklyQuota, 500);
    EXPECT_EQ(weapons->remainingQuota, 500);
}

TEST_F(TradeSystemTests, PanelStaysClosedForBuildingsThatAreNotTradeCenters)
{
    // Given a building that is not a Trade Center.
    const auto commandCenter = place(model::BuildingType::CommandCenter, {0, 0});
    ASSERT_NE(commandCenter, 0U);

    // When the trade panel is requested for it.
    trade_.publishPanel(kPlayer, commandCenter);

    // Then it stays closed.
    ASSERT_EQ(panels_.size(), 1U);
    EXPECT_FALSE(panels_[0].open);
    EXPECT_TRUE(panels_[0].rows.empty());
}

TEST_F(TradeSystemTests, CompletedTradeCenterGetsAQuotaAndStorage)
{
    // Given a Command Center.
    ASSERT_NE(place(model::BuildingType::CommandCenter, {0, 0}), 0U);

    // When a Trade Center is completed.
    const auto center = place(model::BuildingType::TradeCenter, {10, 10});

    // Then it can sell every item and stores the delivered goods.
    ASSERT_NE(center, 0U);
    const auto* quota = world_.tryGet<model::components::TradeQuota>(center);
    ASSERT_NE(quota, nullptr);
    EXPECT_EQ(quota->soldThisWeek[model::itemIndex(model::ItemId::Iron)], 0);
    const auto* inventory = world_.tryGet<model::components::Inventory>(center);
    ASSERT_NE(inventory, nullptr);
    EXPECT_EQ(inventory->slots.size(), 64U);
    EXPECT_EQ(trade_.weeklyQuota(model::ItemId::Ammo), 3000);
}
