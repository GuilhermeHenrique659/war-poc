#pragma once

#include "model/GridPos.hpp"
#include "model/build/BuildingType.hpp"
#include "model/economy/Item.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace model {

enum class Terrain : std::uint8_t { Plain, Water, Blocked };

class BoundedMap {
public:
    BoundedMap(int width, int height, GridPos origin = {});

    [[nodiscard]] bool contains(GridPos tile) const noexcept;
    [[nodiscard]] Terrain terrainAt(GridPos tile) const noexcept;
    bool setTerrain(GridPos tile, Terrain terrain) noexcept;
    [[nodiscard]] int width() const noexcept { return width_; }
    [[nodiscard]] int height() const noexcept { return height_; }
    [[nodiscard]] GridPos origin() const noexcept { return origin_; }
    [[nodiscard]] const std::vector<Terrain>& tiles() const noexcept { return tiles_; }

private:
    [[nodiscard]] std::size_t index(GridPos tile) const noexcept;

    int width_;
    int height_;
    GridPos origin_;
    std::vector<Terrain> tiles_;
};

struct ResourceDeposit {
    ItemId resource = ItemId::Count;
    GridPos origin{};
    int size = 2;

    friend constexpr bool operator==(const ResourceDeposit&, const ResourceDeposit&) = default;
};

inline constexpr int kGeneratedMapWidth = 1024;
inline constexpr int kGeneratedMapHeight = 1024;
inline constexpr GridPos kGeneratedMapOrigin{-512, -512};

class WorldMap {
public:
    WorldMap(std::uint32_t seed, BoundedMap terrain, std::vector<ResourceDeposit> deposits);

    [[nodiscard]] std::uint32_t seed() const noexcept { return seed_; }
    [[nodiscard]] const BoundedMap& terrain() const noexcept { return terrain_; }
    [[nodiscard]] const std::vector<ResourceDeposit>& deposits() const noexcept { return deposits_; }
    [[nodiscard]] const ResourceDeposit* depositAt(GridPos tile) const noexcept;
    [[nodiscard]] bool isWalkable(GridPos tile) const noexcept;
    [[nodiscard]] bool canPlaceBuilding(BuildingType type, GridPos origin, int size) const noexcept;

private:
    std::uint32_t seed_;
    BoundedMap terrain_;
    std::vector<ResourceDeposit> deposits_;
};

[[nodiscard]] WorldMap generateWorldMap(std::uint32_t seed);

} // namespace model
