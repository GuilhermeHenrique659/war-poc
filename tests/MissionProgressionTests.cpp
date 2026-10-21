#include "model/MissionProgression.hpp"

#include <gtest/gtest.h>

TEST(BoundedMapTests, RejectsCoordinatesOutsideConfiguredBounds)
{
    // Given a 4 by 3 map with default passable terrain.
    model::BoundedMap map(4, 3);

    // When querying its edges and an outside tile.
    // Then only coordinates inside the logical rectangle are valid.
    EXPECT_TRUE(map.contains({0, 0}));
    EXPECT_TRUE(map.contains({3, 2}));
    EXPECT_FALSE(map.contains({4, 2}));
    EXPECT_FALSE(map.contains({-1, 0}));
}

TEST(BoundedMapTests, StoresTerrainOnlyInsideTheMap)
{
    // Given a bounded map with plain terrain by default.
    model::BoundedMap map(2, 2, {-1, 4});

    // When terrain is blocked on a contained coordinate and outside the map.
    const bool insideChanged = map.setTerrain({0, 5}, model::Terrain::Blocked);
    const bool outsideChanged = map.setTerrain({1, 5}, model::Terrain::Blocked);

    // Then in-bounds terrain changes are retained and out-of-bounds tiles stay blocked.
    EXPECT_TRUE(insideChanged);
    EXPECT_FALSE(outsideChanged);
    EXPECT_EQ(map.terrainAt({0, 5}), model::Terrain::Blocked);
    EXPECT_EQ(map.terrainAt({-1, 4}), model::Terrain::Plain);
    EXPECT_EQ(map.terrainAt({1, 5}), model::Terrain::Blocked);
}

TEST(MissionProgressionTests, CompletesObjectiveAndAwardsExperienceOnlyOnce)
{
    // Given an active mission requiring two objective points and awarding 120 XP.
    model::Progression progression;
    progression.addPlayer(7);
    model::Mission mission(2, 120);

    // When both points are completed and completion is claimed twice.
    mission.addProgress(1);
    mission.addProgress(1);
    const int firstReward = mission.claimReward(progression, 7);
    const int secondReward = mission.claimReward(progression, 7);

    // Then the mission is won and the player receives the reward once.
    EXPECT_EQ(mission.state(), model::MissionState::Won);
    EXPECT_EQ(firstReward, 120);
    EXPECT_EQ(secondReward, 0);
    EXPECT_EQ(progression.experience(7), 120);
    EXPECT_EQ(progression.level(7), 2);
}

TEST(MissionProgressionTests, TechnologyRequiresLevelAndPrerequisites)
{
    // Given a player at level one and a technology requiring level two.
    model::Progression progression;
    progression.addPlayer(9);
    progression.defineTechnology("advanced_towers", {2, {"basic_towers"}});

    // When checking before and after granting its prerequisite and enough XP.
    EXPECT_FALSE(progression.canUnlock(9, "advanced_towers"));
    progression.defineTechnology("basic_towers", {1, {}});
    progression.unlock(9, "basic_towers");
    progression.grantExperience(9, 100);

    // Then both requirements must be satisfied.
    EXPECT_TRUE(progression.canUnlock(9, "advanced_towers"));
}

TEST(MissionProgressionTests, FailedMissionCannotAwardExperience)
{
    // Given an active mission and a registered player.
    model::Progression progression;
    progression.addPlayer(3);
    model::Mission mission(1, 50);

    // When the mission is failed and its reward is claimed.
    mission.fail();

    // Then no experience is granted.
    EXPECT_EQ(mission.claimReward(progression, 3), 0);
    EXPECT_EQ(progression.experience(3), 0);
}

TEST(MissionProgressionTests, MissingPlayerDoesNotConsumeMissionReward)
{
    // Given a won mission and no registered player.
    model::Progression progression;
    model::Mission mission(1, 50);
    mission.addProgress(1);

    // When the claim is attempted without a player and later retried by a registered player.
    const int missingPlayerClaim = mission.claimReward(progression, 7);
    progression.addPlayer(7);
    const int validPlayerClaim = mission.claimReward(progression, 7);

    // Then only the valid claim consumes and awards the reward.
    EXPECT_EQ(missingPlayerClaim, 0);
    EXPECT_EQ(validPlayerClaim, 50);
    EXPECT_EQ(progression.experience(7), 50);
}
