#pragma once

#include "model/GridPos.hpp"
#include "model/player/Player.hpp"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace model {

enum class Terrain { Plain, Blocked };

// Finite, logical tile rectangle. Terrain defaults to Plain; no rendering data is stored here.
class BoundedMap {
public:
    BoundedMap(int width, int height, GridPos origin = {});

    [[nodiscard]] bool contains(GridPos tile) const noexcept;
    [[nodiscard]] Terrain terrainAt(GridPos tile) const noexcept;
    bool setTerrain(GridPos tile, Terrain terrain) noexcept;
    [[nodiscard]] int width() const noexcept { return width_; }
    [[nodiscard]] int height() const noexcept { return height_; }
    [[nodiscard]] GridPos origin() const noexcept { return origin_; }

private:
    int width_;
    int height_;
    GridPos origin_;
    std::unordered_map<GridPos, Terrain, GridPosHash> terrain_;
};

enum class MissionState { Active, Won, Lost };

class Progression {
public:
    struct TechnologyRequirement {
        int minimumLevel = 1;
        std::vector<std::string> prerequisites;
    };

    void addPlayer(PlayerId player);
    [[nodiscard]] bool hasPlayer(PlayerId player) const noexcept;
    void grantExperience(PlayerId player, int amount);
    [[nodiscard]] int experience(PlayerId player) const noexcept;
    [[nodiscard]] int level(PlayerId player) const noexcept;
    void defineTechnology(std::string id, TechnologyRequirement requirement);
    [[nodiscard]] bool canUnlock(PlayerId player, const std::string& id) const;
    bool unlock(PlayerId player, const std::string& id);
    [[nodiscard]] bool isUnlocked(PlayerId player, const std::string& id) const;

private:
    struct PlayerProgress {
        int experience = 0;
        std::unordered_set<std::string> technologies;
    };

    std::unordered_map<PlayerId, PlayerProgress> players_;
    std::unordered_map<std::string, TechnologyRequirement> technologies_;
};

class Mission {
public:
    Mission(int objectiveTarget, int experienceReward);

    void addProgress(int amount);
    void fail() noexcept;
    [[nodiscard]] int claimReward(Progression& progression, PlayerId player);
    [[nodiscard]] MissionState state() const noexcept { return state_; }
    [[nodiscard]] int objectiveProgress() const noexcept { return progress_; }
    [[nodiscard]] int objectiveTarget() const noexcept { return target_; }

private:
    int target_;
    int reward_;
    int progress_ = 0;
    bool rewardClaimed_ = false;
    MissionState state_ = MissionState::Active;
};

} // namespace model
