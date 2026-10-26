#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/build/BuildingComponents.hpp"
#include "model/build/BuildingSystem.hpp"
#include "model/build/ConstructionComponents.hpp"
#include "model/build/ConstructionSystem.hpp"
#include "model/components/Owner.hpp"
#include "model/economy/InventoryComponents.hpp"
#include "model/economy/InventorySystem.hpp"
#include "model/economy/Item.hpp"
#include "model/player/PlayerComponents.hpp"
#include "model/units/UnitComponents.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <vector>

namespace {

class ConstructionSystemTest : public ::testing::Test {
protected:
    ConstructionSystemTest()
        : buildings_(bus_, world_)
        , construction_(bus_, world_)
    {
        bus_.subscribe<events::BuildingPlaced>([this](const events::BuildingPlaced& event) {
            placements_.push_back(event);
        });
        bus_.subscribe<events::ConstructionStarted>([this](const events::ConstructionStarted& event) {
            sites_.push_back(event);
        });
        bus_.subscribe<events::ConstructionProgressChanged>(
            [this](const events::ConstructionProgressChanged& event) { progress_.push_back(event); });

        const auto playerEntity = world_.create();
        world_.emplace<model::components::PlayerProfile>(playerEntity, kPlayer, "Tester");
        world_.emplace<model::components::Owner>(playerEntity, kPlayer);
        world_.emplace<model::components::Inventory>(playerEntity, std::size_t{32});
    }

    void placeCommandCenter()
    {
        buildings_.select(kPlayer, model::BuildingType::CommandCenter);
        buildings_.place(kPlayer, {0, 0});
    }

    [[nodiscard]] model::ecs::Entity startSite(model::BuildingType type, model::GridPos anchor)
    {
        const std::size_t before = sites_.size();
        buildings_.cancel(kPlayer);
        buildings_.select(kPlayer, type);
        buildings_.place(kPlayer, anchor);
        EXPECT_EQ(sites_.size(), before + 1);
        return sites_.back().entity;
    }

    void deliverAll(model::ecs::Entity entity, model::BuildingType type)
    {
        auto* inventory = world_.tryGet<model::components::Inventory>(entity);
        ASSERT_NE(inventory, nullptr);
        for (const model::BuildingCost& cost : model::buildingCosts(type)) {
            if (cost.quantity <= 0) {
                continue;
            }
            EXPECT_EQ(model::InventorySystem::add(*inventory, cost.item, cost.quantity), cost.quantity);
        }
    }

    void deliver(model::ecs::Entity entity, model::ItemId item, int quantity)
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

    core::EventBus bus_;
    model::ecs::Registry world_;
    model::BuildingSystem buildings_;
    model::ConstructionSystem construction_;
    std::vector<events::BuildingPlaced> placements_;
    std::vector<events::ConstructionStarted> sites_;
    std::vector<events::ConstructionProgressChanged> progress_;
    static constexpr model::PlayerId kPlayer = 1;
};

} // namespace

TEST_F(ConstructionSystemTest, StartsWithTheNominalDurationAndNoProgress)
{
    // Given a Command Center and a newly placed wall.
    placeCommandCenter();
    const auto wall = startSite(model::BuildingType::Wall, {10, 10});

    // Then the site knows its nominal duration and has not advanced.
    const auto* site = world_.tryGet<model::components::ConstructionSite>(wall);
    ASSERT_NE(site, nullptr);
    EXPECT_FLOAT_EQ(site->duration, 3.0F);
    EXPECT_FLOAT_EQ(site->elapsed, 0.0F);
    EXPECT_TRUE(world_.tryGet<model::components::Building>(wall) == nullptr);
}

TEST_F(ConstructionSystemTest, DoesNotAdvanceWithoutMaterials)
{
    // Given a wall site with an empty inventory.
    placeCommandCenter();
    const auto wall = startSite(model::BuildingType::Wall, {10, 10});

    // When time passes.
    construction_.update(5.0F);

    // Then progress pauses and the building is not finished.
    const auto* site = world_.tryGet<model::components::ConstructionSite>(wall);
    ASSERT_NE(site, nullptr);
    EXPECT_FLOAT_EQ(site->elapsed, 0.0F);
    EXPECT_EQ(placements_.size(), 1U);
}

TEST_F(ConstructionSystemTest, ConsumesMaterialsProportionallyToTheProgress)
{
    // Given a wall site with all of its materials.
    placeCommandCenter();
    const auto wall = startSite(model::BuildingType::Wall, {10, 10});
    deliverAll(wall, model::BuildingType::Wall);

    // When half of the nominal duration elapses.
    construction_.update(1.5F);

    // Then half of each material was consumed.
    const auto* site = world_.tryGet<model::components::ConstructionSite>(wall);
    ASSERT_NE(site, nullptr);
    EXPECT_FLOAT_EQ(site->elapsed, 1.5F);
    for (const model::ItemId item : model::kConstructionMaterials) {
        EXPECT_EQ(amountIn(wall, item), 50) << "item " << static_cast<int>(item);
    }
}

TEST_F(ConstructionSystemTest, PausesWhenOneMaterialRunsOutAndResumesAfterDelivery)
{
    // Given a wall site short on iron.
    placeCommandCenter();
    const auto wall = startSite(model::BuildingType::Wall, {10, 10});
    deliverAll(wall, model::BuildingType::Wall);
    auto* inventory = world_.tryGet<model::components::Inventory>(wall);
    ASSERT_NE(inventory, nullptr);
    EXPECT_EQ(model::InventorySystem::remove(*inventory, model::ItemId::Iron, 50), 50);

    // When more than enough time passes.
    construction_.update(3.0F);

    // Then the work stopped when the iron ran out.
    const auto* site = world_.tryGet<model::components::ConstructionSite>(wall);
    ASSERT_NE(site, nullptr);
    EXPECT_FLOAT_EQ(site->elapsed, 1.5F);
    EXPECT_EQ(amountIn(wall, model::ItemId::Iron), 0);
    EXPECT_EQ(placements_.size(), 1U);

    // When no iron is delivered and more time passes.
    construction_.update(2.0F);
    EXPECT_FLOAT_EQ(site->elapsed, 1.5F);

    // When the missing iron arrives.
    deliver(wall, model::ItemId::Iron, 50);
    construction_.update(1.5F);

    // Then the wall finishes and becomes a functional building.
    ASSERT_EQ(placements_.size(), 2U);
    EXPECT_EQ(placements_.back().type, model::BuildingType::Wall);
    EXPECT_TRUE(world_.tryGet<model::components::ConstructionSite>(wall) == nullptr);
    EXPECT_NE(world_.tryGet<model::components::Building>(wall), nullptr);
}

TEST_F(ConstructionSystemTest, DebugModeFinishesWithoutMaterials)
{
    // Given debug mode and an empty wall site.
    placeCommandCenter();
    const auto wall = startSite(model::BuildingType::Wall, {10, 10});
    bus_.publish(events::DebugToggleRequested{});

    // When the nominal duration elapses.
    construction_.update(3.0F);

    // Then the building is finished even without materials.
    EXPECT_TRUE(world_.tryGet<model::components::ConstructionSite>(wall) == nullptr);
    EXPECT_NE(world_.tryGet<model::components::Building>(wall), nullptr);
    EXPECT_EQ(placements_.size(), 2U);
}

TEST_F(ConstructionSystemTest, PublishesProgressOnlyWhenTheWholePercentageChanges)
{
    // Given a fully supplied wall.
    placeCommandCenter();
    const auto wall = startSite(model::BuildingType::Wall, {10, 10});
    deliverAll(wall, model::BuildingType::Wall);
    progress_.clear();

    // When time advances in small steps until completion.
    for (int step = 0; step < 300; ++step) {
        construction_.update(0.01F);
    }

    // Then progress events are monotonic, bounded and end at 100%.
    ASSERT_FALSE(progress_.empty());
    EXPECT_LE(progress_.size(), 101U);
    float previous = 0.0F;
    for (const events::ConstructionProgressChanged& event : progress_) {
        EXPECT_EQ(event.entity, wall);
        EXPECT_GE(event.progress, previous);
        previous = event.progress;
    }
    EXPECT_FLOAT_EQ(progress_.back().progress, 1.0F);
}

TEST_F(ConstructionSystemTest, IgnoresNonPositiveAndNonFiniteDeltas)
{
    // Given a fully supplied wall.
    placeCommandCenter();
    const auto wall = startSite(model::BuildingType::Wall, {10, 10});
    deliverAll(wall, model::BuildingType::Wall);

    // When invalid deltas are applied.
    construction_.update(0.0F);
    construction_.update(-1.0F);
    construction_.update(std::numeric_limits<float>::quiet_NaN());

    // Then nothing advances.
    const auto* site = world_.tryGet<model::components::ConstructionSite>(wall);
    ASSERT_NE(site, nullptr);
    EXPECT_FLOAT_EQ(site->elapsed, 0.0F);
}

TEST_F(ConstructionSystemTest, CompletesOnceAndPublishesTheBuildingPlaced)
{
    // Given a fully supplied wall.
    placeCommandCenter();
    const auto wall = startSite(model::BuildingType::Wall, {10, 10});
    deliverAll(wall, model::BuildingType::Wall);

    // When more time than needed passes.
    construction_.update(10.0F);

    // Then exactly one BuildingPlaced notification carries the finished wall.
    ASSERT_EQ(placements_.size(), 2U);
    const events::BuildingPlaced& placed = placements_.back();
    EXPECT_EQ(placed.entity, wall);
    EXPECT_EQ(placed.owner, kPlayer);
    EXPECT_EQ(placed.type, model::BuildingType::Wall);
    EXPECT_EQ(placed.origin, (model::GridPos{10, 10}));
    EXPECT_EQ(placed.size, 1);
    EXPECT_NE(world_.tryGet<model::components::Building>(wall), nullptr);
}
