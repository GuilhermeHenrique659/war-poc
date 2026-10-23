#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/PlayingModel.hpp"

#include <gtest/gtest.h>

#include <vector>

TEST(PlayingModelMapTests, PublishesTheSeededWorldAndRequiresMatchingDepositsForMines)
{
    // Given a new match and observers for map and building state.
    core::EventBus bus;
    model::PlayingModel playing(bus);
    events::MapGenerated generatedMap{};
    bool receivedMap = false;
    std::vector<events::BuildingPlaced> buildings;
    events::BuildPreviewChanged preview{};
    bus.subscribe<events::MapGenerated>([&](const auto& event) {
        generatedMap = event;
        receivedMap = true;
    });
    bus.subscribe<events::BuildingPlaced>([&](const auto& event) { buildings.push_back(event); });
    bus.subscribe<events::BuildPreviewChanged>([&](const auto& event) { preview = event; });

    // When the match starts and the player builds a Command Center.
    bus.publish(events::StartMatchRequested{"Jogador"});
    bus.publish(events::BuildSelectRequested{1, model::BuildingType::CommandCenter});
    bus.publish(events::BuildPlaceRequested{1, {0, 0}});

    // Then the map is delivered and a copper deposit cannot host an iron mine.
    ASSERT_TRUE(receivedMap);
    EXPECT_EQ(generatedMap.width, 1024);
    EXPECT_EQ(generatedMap.height, 1024);
    EXPECT_EQ(generatedMap.terrain.size(), 1024U * 1024U);
    ASSERT_EQ(buildings.size(), 1U);
    bus.publish(events::BuildSelectRequested{1, model::BuildingType::IronMine});
    bus.publish(events::BuildCursorMoved{1, model::GridPos{-5, 0}});
    EXPECT_FALSE(preview.valid);
    bus.publish(events::BuildPlaceRequested{1, {-5, 0}});
    EXPECT_EQ(buildings.size(), 1U);

    // When the iron mine is placed over an iron deposit.
    bus.publish(events::BuildCursorMoved{1, model::GridPos{-9, 0}});
    EXPECT_TRUE(preview.valid);
    bus.publish(events::BuildPlaceRequested{1, {-9, 0}});

    // Then the matching mine is created.
    ASSERT_EQ(buildings.size(), 2U);
    EXPECT_EQ(buildings.back().type, model::BuildingType::IronMine);
    EXPECT_EQ(buildings.back().origin, (model::GridPos{-10, -1}));
}
