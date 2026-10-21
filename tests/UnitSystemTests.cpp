#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/build/BuildingSystem.hpp"
#include "model/units/UnitType.hpp"
#include "model/units/UnitSystem.hpp"

#include <gtest/gtest.h>

#include <vector>

namespace {

class UnitSystemTest : public ::testing::Test {
protected:
    UnitSystemTest()
        : buildings_(bus_, world_)
        , units_(bus_, world_, buildings_)
    {
        bus_.subscribe<events::UnitSpawned>([this](const events::UnitSpawned& event) {
            spawns_.push_back(event);
        });
        bus_.subscribe<events::UnitMoved>([this](const events::UnitMoved& event) {
            moves_.push_back(event);
        });
    }

    core::EventBus bus_;
    model::ecs::Registry world_;
    model::BuildingSystem buildings_;
    model::UnitSystem units_;
    std::vector<events::UnitSpawned> spawns_;
    std::vector<events::UnitMoved> moves_;
    static constexpr model::PlayerId kPlayer = 1;
};

} // namespace

TEST_F(UnitSystemTest, MovesAnOwnedUnitToTheRequestedFreeDestination)
{
    // Given a Soldier unit standing at the origin.
    const model::UnitStats soldier = model::recruitStats(model::UnitType::Soldier);
    const model::ecs::Entity unit = units_.spawn(kPlayer, soldier, {0, 0});
    ASSERT_EQ(spawns_.size(), 1U);

    // When its owner orders it to move two tiles east and time advances by one second.
    const model::GridPos destination{2, 0};
    units_.orderMove(kPlayer, {unit}, destination);
    EXPECT_TRUE(units_.update(1.0F));

    // Then the unit reaches and reserves that destination, emitting a movement event.
    const auto* position = world_.tryGet<model::components::Position>(unit);
    ASSERT_NE(position, nullptr);
    EXPECT_FLOAT_EQ(position->x, 2.5F);
    EXPECT_FLOAT_EQ(position->y, 0.5F);
    EXPECT_TRUE(units_.isTileTaken(destination));
    ASSERT_EQ(moves_.size(), 1U);
    EXPECT_EQ(moves_.front().entity, unit);
}

TEST_F(UnitSystemTest, DoesNotPathfindThroughBlockedTerrain)
{
    // Given a unit separated from its destination by a wall of blocked tiles.
    units_.setTileWalkable([](model::GridPos tile) { return tile.x != 1; });
    const model::UnitStats soldier = model::recruitStats(model::UnitType::Soldier);
    const model::ecs::Entity unit = units_.spawn(kPlayer, soldier, {0, 0});

    // When it is ordered to move across the wall.
    units_.orderMove(kPlayer, {unit}, {2, 0});
    (void)units_.update(1.0F);

    // Then it remains on its original tile.
    const auto* position = world_.tryGet<model::components::Position>(unit);
    ASSERT_NE(position, nullptr);
    EXPECT_FLOAT_EQ(position->x, 0.5F);
    EXPECT_FLOAT_EQ(position->y, 0.5F);
}

TEST(UnitNameTests, ShowsTheNewRecruitableNames)
{
    // Given the three recruitable unit types.
    const model::UnitSpec soldier = model::unitSpec(model::UnitType::Soldier);
    const model::UnitSpec bat = model::unitSpec(model::UnitType::Bat);
    const model::UnitSpec goat = model::unitSpec(model::UnitType::Goat);

    // When their presentation labels are read.

    // Then the initial units are named Soldado, Morcego, and Bode.
    EXPECT_EQ(soldier.label, "Soldado");
    EXPECT_EQ(bat.label, "Morcego");
    EXPECT_EQ(goat.label, "Bode");
}
