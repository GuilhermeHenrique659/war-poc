#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/BuildingSystem.hpp"
#include "model/MergeSystem.hpp"
#include "model/SelectionSystem.hpp"
#include "model/UnitSystem.hpp"

#include <gtest/gtest.h>

#include <vector>

namespace {

class MergeSystemTest : public ::testing::Test {
protected:
    MergeSystemTest()
        : buildings_(bus_, world_)
        , units_(bus_, world_, buildings_)
        , selection_(bus_, world_)
        , merges_(bus_, world_, units_, selection_)
    {
        bus_.subscribe<events::MergePanelChanged>([this](const events::MergePanelChanged& event) {
            panels_.push_back(event);
        });
        bus_.subscribe<events::UnitSpawned>([this](const events::UnitSpawned& event) {
            spawns_.push_back(event);
        });
        bus_.subscribe<events::UnitRemoved>([this](const events::UnitRemoved& event) {
            removals_.push_back(event);
        });
    }

    core::EventBus bus_;
    model::ecs::Registry world_;
    model::BuildingSystem buildings_;
    model::UnitSystem units_;
    model::SelectionSystem selection_;
    model::MergeSystem merges_;
    std::vector<events::MergePanelChanged> panels_;
    std::vector<events::UnitSpawned> spawns_;
    std::vector<events::UnitRemoved> removals_;
    static constexpr model::PlayerId kPlayer = 1;
};

} // namespace

TEST_F(MergeSystemTest, CombinesSelectedNearbyUnitsAndSelectsTheResult)
{
    // Given two nearby selected infantry units.
    const model::UnitStats infantry = model::recruitStats(model::UnitType::Infantry);
    const model::ecs::Entity first = units_.spawn(kPlayer, infantry, {0, 0});
    const model::ecs::Entity second = units_.spawn(kPlayer, infantry, {1, 0});
    selection_.selectUnits(kPlayer, {first, second}, false);

    // When the merge panel is refreshed and the player confirms the merge.
    merges_.refresh(kPlayer);
    ASSERT_EQ(panels_.size(), 1U);
    const events::MergePanelChanged preview = panels_.back();
    merges_.merge(kPlayer);

    // Then the preview and result use the combined stats, and only the mech remains selected.
    EXPECT_TRUE(preview.open);
    EXPECT_TRUE(preview.canMerge);
    EXPECT_EQ(preview.unitCount, 2);
    EXPECT_EQ(preview.maxHealth, 200);
    EXPECT_EQ(preview.damage, 20);
    EXPECT_FLOAT_EQ(preview.range, 1.0F);
    EXPECT_FLOAT_EQ(preview.speed, 2.0F);
    EXPECT_EQ(preview.size, 2);
    EXPECT_FALSE(world_.isAlive(first));
    EXPECT_FALSE(world_.isAlive(second));
    ASSERT_EQ(removals_.size(), 2U);
    ASSERT_EQ(spawns_.size(), 3U);

    const model::ecs::Entity combined = spawns_.back().entity;
    const auto* unit = world_.tryGet<model::components::Unit>(combined);
    const auto* health = world_.tryGet<model::components::Health>(combined);
    ASSERT_NE(unit, nullptr);
    ASSERT_NE(health, nullptr);
    EXPECT_EQ(unit->type, model::UnitType::Combined);
    EXPECT_EQ(unit->baseCount, 2);
    EXPECT_EQ(unit->size, 2);
    EXPECT_EQ(health->current, 200);
    EXPECT_EQ(health->max, 200);

    const std::vector<model::ecs::Entity> selected = selection_.selectedUnits(kPlayer);
    ASSERT_EQ(selected.size(), 1U);
    EXPECT_EQ(selected.front(), combined);
}
