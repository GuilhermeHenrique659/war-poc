#include "model/WorldMap.hpp"
#include "model/build/BuildingType.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <ranges>

namespace {

[[nodiscard]] model::GridPos findTerrainTile(const model::BoundedMap& terrain, model::Terrain target)
{
    for (int y = terrain.origin().y; y < terrain.origin().y + terrain.height(); ++y) {
        for (int x = terrain.origin().x; x < terrain.origin().x + terrain.width(); ++x) {
            const model::GridPos tile{x, y};
            if (terrain.terrainAt(tile) == target) {
                return tile;
            }
        }
    }
    return {};
}

} // namespace

TEST(WorldMapTests, GeneratesTheSameTerrainAndDepositsForTheSameSeed)
{
    // Given a fixed seed.
    constexpr std::uint32_t seed = 123456U;

    // When two maps are generated from it.
    const model::WorldMap first = model::generateWorldMap(seed);
    const model::WorldMap second = model::generateWorldMap(seed);

    // Then their generated world state is identical.
    EXPECT_EQ(first.seed(), seed);
    EXPECT_EQ(second.seed(), seed);
    EXPECT_EQ(first.terrain().tiles(), second.terrain().tiles());
    EXPECT_EQ(first.deposits(), second.deposits());
}

TEST(WorldMapTests, ChangesTheGeneratedWorldWhenTheSeedChanges)
{
    // Given two different seeds.
    const model::WorldMap first = model::generateWorldMap(1U);
    const model::WorldMap second = model::generateWorldMap(2U);

    // When their generated terrain is compared.
    // Then the maps are not identical.
    EXPECT_NE(first.terrain().tiles(), second.terrain().tiles());
}

TEST(WorldMapTests, GeneratesTheConfiguredFiniteMapAndWaterCoverage)
{
    // Given a generated map.
    const model::WorldMap map = model::generateWorldMap(7U);

    // When checking its bounds and terrain distribution.
    const auto& terrain = map.terrain();
    const auto waterTiles = std::ranges::count(terrain.tiles(), model::Terrain::Water);

    // Then it has the configured size, blocks the outside, and targets 30% water.
    EXPECT_EQ(terrain.width(), 1024);
    EXPECT_EQ(terrain.height(), 1024);
    EXPECT_TRUE(terrain.contains({0, 0}));
    EXPECT_FALSE(terrain.contains({1024, 0}));
    EXPECT_EQ(terrain.terrainAt({1024, 0}), model::Terrain::Blocked);
    EXPECT_GT(waterTiles, 1024 * 1024 * 28 / 100);
    EXPECT_LT(waterTiles, 1024 * 1024 * 32 / 100);
    const model::GridPos water = findTerrainTile(terrain, model::Terrain::Water);
    EXPECT_FALSE(map.isWalkable(water));
    EXPECT_FALSE(map.canPlaceBuilding(model::BuildingType::House, water, 2));
}

TEST(WorldMapTests, PlacesTheGuaranteedStartingDepositsOnLand)
{
    // Given a generated map.
    const model::WorldMap map = model::generateWorldMap(42U);

    // When inspecting the guaranteed deposits around the logical origin.
    const auto* firstIron = map.depositAt({-10, -1});
    const auto* secondIron = map.depositAt({-8, -1});
    const auto* copper = map.depositAt({-6, -1});
    const auto* firstCoal = map.depositAt({6, -1});
    const auto* secondCoal = map.depositAt({8, -1});

    // Then each 2x2 deposit has the correct resource and all its tiles are land.
    ASSERT_NE(firstIron, nullptr);
    ASSERT_NE(secondIron, nullptr);
    ASSERT_NE(copper, nullptr);
    ASSERT_NE(firstCoal, nullptr);
    ASSERT_NE(secondCoal, nullptr);
    EXPECT_TRUE(map.canPlaceBuilding(model::BuildingType::CommandCenter, {-2, -2}, 5));
    EXPECT_TRUE(map.canPlaceBuilding(model::BuildingType::IronMine, firstIron->origin, firstIron->size));
    EXPECT_FALSE(map.canPlaceBuilding(model::BuildingType::CopperMine, firstIron->origin, firstIron->size));
    EXPECT_FALSE(map.canPlaceBuilding(model::BuildingType::IronMine, copper->origin, copper->size));
    EXPECT_FALSE(map.canPlaceBuilding(model::BuildingType::IronMine, {20, 20}, 2));
    EXPECT_TRUE(map.canPlaceBuilding(model::BuildingType::CopperMine, copper->origin, copper->size));
    EXPECT_TRUE(map.canPlaceBuilding(model::BuildingType::CoalMine, firstCoal->origin, firstCoal->size));
    EXPECT_EQ(firstIron->resource, model::ItemId::Iron);
    EXPECT_EQ(secondIron->resource, model::ItemId::Iron);
    EXPECT_EQ(copper->resource, model::ItemId::Copper);
    EXPECT_EQ(firstCoal->resource, model::ItemId::Coal);
    EXPECT_EQ(secondCoal->resource, model::ItemId::Coal);
    for (const model::GridPos origin : {firstIron->origin, secondIron->origin, copper->origin,
             firstCoal->origin, secondCoal->origin}) {
        for (int y = 0; y < 2; ++y) {
            for (int x = 0; x < 2; ++x) {
                EXPECT_EQ(map.terrain().terrainAt({origin.x + x, origin.y + y}), model::Terrain::Plain);
            }
        }
    }
}

TEST(WorldMapTests, GeneratesFiftyAdditionalDepositsOfEachResourceWithoutOverlaps)
{
    // Given a generated map with guaranteed starting deposits.
    const model::WorldMap map = model::generateWorldMap(99U);

    // When counting deposits by resource and checking their footprints.
    const auto count = [&map](model::ItemId resource) {
        return std::ranges::count_if(map.deposits(), [resource](const model::ResourceDeposit& deposit) {
            return deposit.resource == resource;
        });
    };

    // Then totals include the five starting deposits and every footprint is unique and land.
    EXPECT_EQ(count(model::ItemId::Iron), 52);
    EXPECT_EQ(count(model::ItemId::Copper), 51);
    EXPECT_EQ(count(model::ItemId::Coal), 52);
    for (std::size_t i = 0; i < map.deposits().size(); ++i) {
        const auto& deposit = map.deposits()[i];
        EXPECT_EQ(deposit.size, 2);
        for (int y = 0; y < deposit.size; ++y) {
            for (int x = 0; x < deposit.size; ++x) {
                EXPECT_EQ(map.terrain().terrainAt({deposit.origin.x + x, deposit.origin.y + y}),
                    model::Terrain::Plain);
            }
        }
        for (std::size_t other = i + 1; other < map.deposits().size(); ++other) {
            const auto& compared = map.deposits()[other];
            const bool overlap = deposit.origin.x < compared.origin.x + compared.size
                && compared.origin.x < deposit.origin.x + deposit.size
                && deposit.origin.y < compared.origin.y + compared.size
                && compared.origin.y < deposit.origin.y + deposit.size;
            EXPECT_FALSE(overlap);
        }
    }
}

TEST(WorldMapTests, RequiresLandBesideTheCommandCenterForThePlayerSpawn)
{
    // Given a map whose land is entirely occupied by a Command Center footprint.
    const model::WorldMap map(0U, model::BoundedMap(5, 5), {});

    // When the placement rule checks the full map footprint.
    const bool canPlace = map.canPlaceBuilding(model::BuildingType::CommandCenter, {0, 0}, 5);

    // Then it rejects the placement because the player cannot spawn on adjacent land.
    EXPECT_FALSE(canPlace);
}
