#include "model/MissionProgression.hpp"

#include <algorithm>
#include <ranges>

namespace model {

void Progression::addPlayer(PlayerId player)
{
    if (player != kNoPlayer) {
        players_.try_emplace(player);
    }
}

bool Progression::hasPlayer(PlayerId player) const noexcept
{
    return players_.contains(player);
}

void Progression::grantExperience(PlayerId player, int amount)
{
    const auto found = players_.find(player);
    if (found == players_.end() || amount <= 0) {
        return;
    }
    found->second.experience += amount;
}

int Progression::experience(PlayerId player) const noexcept
{
    const auto found = players_.find(player);
    return found == players_.end() ? 0 : found->second.experience;
}

int Progression::level(PlayerId player) const noexcept
{
    if (!players_.contains(player)) {
        return 0;
    }
    int currentLevel = 1;
    int remainder = experience(player);
    while (remainder >= currentLevel * 100) {
        remainder -= currentLevel * 100;
        ++currentLevel;
    }
    return currentLevel;
}

void Progression::defineTechnology(std::string id, TechnologyRequirement requirement)
{
    if (id.empty() || requirement.minimumLevel < 1) {
        return;
    }
    technologies_.insert_or_assign(std::move(id), std::move(requirement));
}

bool Progression::canUnlock(PlayerId player, const std::string& id) const
{
    const auto progress = players_.find(player);
    const auto technology = technologies_.find(id);
    if (progress == players_.end() || technology == technologies_.end()
        || progress->second.technologies.contains(id)
        || level(player) < technology->second.minimumLevel) {
        return false;
    }
    return std::ranges::all_of(technology->second.prerequisites, [&](const std::string& prerequisite) {
        return progress->second.technologies.contains(prerequisite);
    });
}

bool Progression::unlock(PlayerId player, const std::string& id)
{
    if (!canUnlock(player, id)) {
        return false;
    }
    players_.at(player).technologies.insert(id);
    return true;
}

bool Progression::isUnlocked(PlayerId player, const std::string& id) const
{
    const auto found = players_.find(player);
    return found != players_.end() && found->second.technologies.contains(id);
}

Mission::Mission(int objectiveTarget, int experienceReward)
    : target_(std::max(1, objectiveTarget))
    , reward_(std::max(0, experienceReward))
{
}

void Mission::addProgress(int amount)
{
    if (state_ != MissionState::Active || amount <= 0) {
        return;
    }
    progress_ += std::min(amount, target_ - progress_);
    if (progress_ == target_) {
        state_ = MissionState::Won;
    }
}

void Mission::fail() noexcept
{
    if (state_ == MissionState::Active) {
        state_ = MissionState::Lost;
    }
}

int Mission::claimReward(Progression& progression, PlayerId player)
{
    if (state_ != MissionState::Won || rewardClaimed_ || !progression.hasPlayer(player)) {
        return 0;
    }
    rewardClaimed_ = true;
    progression.grantExperience(player, reward_);
    return reward_;
}

} // namespace model
