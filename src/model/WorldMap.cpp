#include "model/WorldMap.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <utility>

namespace model {
namespace {

constexpr int kStartingAreaRadius = 16;
constexpr int kWaterRadiusMin = 10;
constexpr int kWaterRadiusMax = 35;
constexpr int kExtraDepositsPerResource = 50;
constexpr int kDepositSize = 2;
constexpr std::uint64_t kMaxDepositAttempts = 1000000;

[[nodiscard]] bool inStartingArea(GridPos tile)
{
    return std::max(std::abs(tile.x), std::abs(tile.y)) <= kStartingAreaRadius;
}

[[nodiscard]] ItemId mineResource(BuildingType type) noexcept
{
    switch (type) {
    case BuildingType::IronMine: return ItemId::Iron;
    case BuildingType::CopperMine: return ItemId::Copper;
    case BuildingType::CoalMine: return ItemId::Coal;
    default: return ItemId::Count;
    }
}

[[nodiscard]] bool footprintIsPlain(const BoundedMap& map, GridPos origin, int size)
{
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            if (map.terrainAt({origin.x + x, origin.y + y}) != Terrain::Plain) {
                return false;
            }
        }
    }
    return true;
}

[[nodiscard]] bool overlapsDeposit(
    const std::vector<ResourceDeposit>& deposits, GridPos origin, int size)
{
    for (const ResourceDeposit& deposit : deposits) {
        const bool separated = origin.x + size <= deposit.origin.x || deposit.origin.x + deposit.size <= origin.x
            || origin.y + size <= deposit.origin.y || deposit.origin.y + deposit.size <= origin.y;
        if (!separated) {
            return true;
        }
    }
    return false;
}

void addWater(BoundedMap& map, std::uint64_t targetWaterTiles, std::mt19937& random)
{
    std::uniform_int_distribution<int> xDistribution(map.origin().x, map.origin().x + map.width() - 1);
    std::uniform_int_distribution<int> yDistribution(map.origin().y, map.origin().y + map.height() - 1);
    std::uniform_int_distribution<int> radiusDistribution(kWaterRadiusMin, kWaterRadiusMax);
    std::uint64_t waterTiles = 0;

    while (waterTiles < targetWaterTiles) {
        const GridPos center{xDistribution(random), yDistribution(random)};
        const int radius = radiusDistribution(random);
        const int left = std::max(map.origin().x, center.x - radius);
        const int top = std::max(map.origin().y, center.y - radius);
        const int right = std::min(map.origin().x + map.width() - 1, center.x + radius);
        const int bottom = std::min(map.origin().y + map.height() - 1, center.y + radius);
        const std::int64_t radiusSquared = static_cast<std::int64_t>(radius) * radius;
        for (int y = top; y <= bottom; ++y) {
            for (int x = left; x <= right; ++x) {
                const GridPos tile{x, y};
                if (inStartingArea(tile)) {
                    continue;
                }
                const std::int64_t dx = static_cast<std::int64_t>(x) - center.x;
                const std::int64_t dy = static_cast<std::int64_t>(y) - center.y;
                if (dx * dx + dy * dy <= radiusSquared && map.terrainAt(tile) == Terrain::Plain) {
                    (void)map.setTerrain(tile, Terrain::Water);
                    ++waterTiles;
                }
            }
        }
    }
}

[[nodiscard]] std::vector<ResourceDeposit> generateDeposits(const BoundedMap& terrain, std::mt19937& random)
{
    std::vector<ResourceDeposit> deposits{
        {ItemId::Iron, {-10, -1}, kDepositSize},
        {ItemId::Iron, {-8, -1}, kDepositSize},
        {ItemId::Copper, {-6, -1}, kDepositSize},
        {ItemId::Coal, {6, -1}, kDepositSize},
        {ItemId::Coal, {8, -1}, kDepositSize},
    };

    std::uniform_int_distribution<int> xDistribution(
        terrain.origin().x, terrain.origin().x + terrain.width() - kDepositSize);
    std::uniform_int_distribution<int> yDistribution(
        terrain.origin().y, terrain.origin().y + terrain.height() - kDepositSize);
    constexpr ItemId resources[]{ItemId::Iron, ItemId::Copper, ItemId::Coal};
    std::uint64_t attempts = 0;
    for (const ItemId resource : resources) {
        int placed = 0;
        while (placed < kExtraDepositsPerResource) {
            if (++attempts > kMaxDepositAttempts) {
                throw std::runtime_error("Unable to place all resource deposits on the generated map.");
            }
            const GridPos origin{xDistribution(random), yDistribution(random)};
            if (inStartingArea(origin) || !footprintIsPlain(terrain, origin, kDepositSize)
                || overlapsDeposit(deposits, origin, kDepositSize)) {
                continue;
            }
            deposits.push_back(ResourceDeposit{resource, origin, kDepositSize});
            ++placed;
        }
    }
    return deposits;
}

} // namespace

BoundedMap::BoundedMap(int width, int height, GridPos origin)
    : width_(std::max(0, width))
    , height_(std::max(0, height))
    , origin_(origin)
    , tiles_(static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_), Terrain::Plain)
{
}

bool BoundedMap::contains(GridPos tile) const noexcept
{
    const auto dx = static_cast<std::int64_t>(tile.x) - origin_.x;
    const auto dy = static_cast<std::int64_t>(tile.y) - origin_.y;
    return dx >= 0 && dy >= 0 && dx < width_ && dy < height_;
}

Terrain BoundedMap::terrainAt(GridPos tile) const noexcept
{
    return contains(tile) ? tiles_[index(tile)] : Terrain::Blocked;
}

bool BoundedMap::setTerrain(GridPos tile, Terrain terrain) noexcept
{
    if (!contains(tile)) {
        return false;
    }
    tiles_[index(tile)] = terrain;
    return true;
}

std::size_t BoundedMap::index(GridPos tile) const noexcept
{
    const auto dx = static_cast<std::size_t>(static_cast<std::int64_t>(tile.x) - origin_.x);
    const auto dy = static_cast<std::size_t>(static_cast<std::int64_t>(tile.y) - origin_.y);
    return dy * static_cast<std::size_t>(width_) + dx;
}

WorldMap::WorldMap(std::uint32_t seed, BoundedMap terrain, std::vector<ResourceDeposit> deposits)
    : seed_(seed)
    , terrain_(std::move(terrain))
    , deposits_(std::move(deposits))
{
}

const ResourceDeposit* WorldMap::depositAt(GridPos tile) const noexcept
{
    for (const ResourceDeposit& deposit : deposits_) {
        if (tile.x >= deposit.origin.x && tile.y >= deposit.origin.y
            && tile.x < deposit.origin.x + deposit.size && tile.y < deposit.origin.y + deposit.size) {
            return &deposit;
        }
    }
    return nullptr;
}

bool WorldMap::isWalkable(GridPos tile) const noexcept
{
    return terrain_.terrainAt(tile) == Terrain::Plain;
}

bool WorldMap::canPlaceBuilding(BuildingType type, GridPos origin, int size) const noexcept
{
    if (size <= 0 || !footprintIsPlain(terrain_, origin, size)) {
        return false;
    }

    if (type == BuildingType::CommandCenter) {
        for (int y = origin.y - 1; y <= origin.y + size; ++y) {
            for (int x = origin.x - 1; x <= origin.x + size; ++x) {
                const bool insideFootprint
                    = x >= origin.x && x < origin.x + size && y >= origin.y && y < origin.y + size;
                if (!insideFootprint && isWalkable({x, y})) {
                    return true;
                }
            }
        }
        return false;
    }

    const ItemId requiredResource = mineResource(type);
    if (requiredResource == ItemId::Count) {
        return true;
    }
    const ResourceDeposit* deposit = depositAt(origin);
    return deposit != nullptr && deposit->origin == origin && deposit->size == size
        && deposit->resource == requiredResource;
}

WorldMap generateWorldMap(std::uint32_t seed)
{
    BoundedMap terrain(kGeneratedMapWidth, kGeneratedMapHeight, kGeneratedMapOrigin);
    const auto tileCount = static_cast<std::uint64_t>(terrain.width()) * static_cast<std::uint64_t>(terrain.height());
    std::mt19937 random(seed);
    addWater(terrain, tileCount * 30U / 100U, random);
    std::vector<ResourceDeposit> deposits = generateDeposits(terrain, random);
    return WorldMap(seed, std::move(terrain), std::move(deposits));
}

} // namespace model
