#include "view/ui/GameText.hpp"

#include "view/ui/TextCatalog.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace view::ui {

namespace {

constexpr std::size_t kCommonCostItemCount = 6;

[[nodiscard]] bool isCommonCost(const std::vector<events::CostEntry>& costs)
{
    if (costs.size() != kCommonCostItemCount) {
        return false;
    }
    const int quantity = costs.front().quantity;
    for (const events::CostEntry& cost : costs) {
        if (cost.quantity != quantity) {
            return false;
        }
    }
    return true;
}

} // namespace

std::string_view buildingName(model::BuildingType type)
{
    switch (type) {
    case model::BuildingType::CommandCenter: return text(TextId::BuildingCommandCenter);
    case model::BuildingType::Wall: return text(TextId::BuildingWall);
    case model::BuildingType::House: return text(TextId::BuildingHouse);
    case model::BuildingType::Generator: return text(TextId::BuildingGenerator);
    case model::BuildingType::Warehouse: return text(TextId::BuildingWarehouse);
    case model::BuildingType::Barracks: return text(TextId::BuildingBarracks);
    case model::BuildingType::Cruzador: return text(TextId::BuildingCruzador);
    case model::BuildingType::Tower: return text(TextId::BuildingTower);
    case model::BuildingType::IronMine: return text(TextId::BuildingIronMine);
    case model::BuildingType::CoalMine: return text(TextId::BuildingCoalMine);
    case model::BuildingType::Steelworks: return text(TextId::BuildingSteelworks);
    case model::BuildingType::CopperMine: return text(TextId::BuildingCopperMine);
    case model::BuildingType::ElectronicsFactory: return text(TextId::BuildingElectronicsFactory);
    case model::BuildingType::ToolFactory: return text(TextId::BuildingToolFactory);
    case model::BuildingType::WeaponFactory: return text(TextId::BuildingWeaponFactory);
    case model::BuildingType::AmmoFactory: return text(TextId::BuildingAmmoFactory);
    case model::BuildingType::TrainingDummy: return text(TextId::BuildingTrainingDummy);
    }
    return text(TextId::CommonUnknown);
}

std::string_view unitName(model::UnitType type)
{
    switch (type) {
    case model::UnitType::Soldier: return text(TextId::UnitSoldier);
    case model::UnitType::Bat: return text(TextId::UnitBat);
    case model::UnitType::Goat: return text(TextId::UnitGoat);
    case model::UnitType::Combined: return text(TextId::UnitCombined);
    }
    return text(TextId::CommonUnknown);
}

std::string_view itemName(model::ItemId item)
{
    switch (item) {
    case model::ItemId::Iron: return text(TextId::ItemIron);
    case model::ItemId::Coal: return text(TextId::ItemCoal);
    case model::ItemId::Steel: return text(TextId::ItemSteel);
    case model::ItemId::Copper: return text(TextId::ItemCopper);
    case model::ItemId::Electronics: return text(TextId::ItemElectronics);
    case model::ItemId::Tools: return text(TextId::ItemTools);
    case model::ItemId::Weapon: return text(TextId::ItemWeapon);
    case model::ItemId::Ammo: return text(TextId::ItemAmmo);
    case model::ItemId::Count: break;
    }
    return text(TextId::CommonUnknown);
}

std::string_view shortItemName(model::ItemId item)
{
    switch (item) {
    case model::ItemId::Iron: return text(TextId::ItemShortIron);
    case model::ItemId::Coal: return text(TextId::ItemShortCoal);
    case model::ItemId::Steel: return text(TextId::ItemShortSteel);
    case model::ItemId::Copper: return text(TextId::ItemShortCopper);
    case model::ItemId::Electronics: return text(TextId::ItemShortElectronics);
    case model::ItemId::Tools: return text(TextId::ItemShortTools);
    case model::ItemId::Weapon: return text(TextId::ItemShortWeapon);
    case model::ItemId::Ammo: return text(TextId::ItemShortAmmo);
    case model::ItemId::Count: break;
    }
    return text(TextId::CommonUnknown);
}

std::string_view menuLabel(model::MenuAction action)
{
    switch (action) {
    case model::MenuAction::StartGame: return text(TextId::MenuStart);
    case model::MenuAction::Settings: return text(TextId::MenuSettings);
    case model::MenuAction::Quit: return text(TextId::MenuQuit);
    }
    return text(TextId::CommonUnknown);
}

std::string_view sceneName(model::Scene scene)
{
    switch (scene) {
    case model::Scene::MainMenu: return text(TextId::SceneMenu);
    case model::Scene::Settings: return text(TextId::SceneSettings);
    case model::Scene::Playing: return text(TextId::ScenePlaying);
    }
    return text(TextId::CommonUnknown);
}

std::string_view lockReason(events::BuildLockReason reason)
{
    switch (reason) {
    case events::BuildLockReason::None: return {};
    case events::BuildLockReason::AlreadyBuilt: return text(TextId::LockAlreadyBuilt);
    case events::BuildLockReason::RequiresCommandCenter: return text(TextId::LockRequiresCommandCenter);
    }
    return {};
}

std::string dateText(const events::TimeChanged& time)
{
    return format(TextId::HudMonthWeek, time.month, time.week);
}

std::string dayClockText(const events::TimeChanged& time)
{
    const std::uint32_t minutes = time.tickInDay / 60U;
    const std::uint32_t seconds = time.tickInDay % 60U;
    return format(TextId::HudDayClock, minutes, seconds, time.day);
}

std::string costText(const std::vector<events::CostEntry>& costs, bool free)
{
    if (free) {
        return text(TextId::CommonFree);
    }

    std::vector<events::CostEntry> positive;
    positive.reserve(costs.size());
    for (const events::CostEntry& cost : costs) {
        if (cost.quantity > 0) {
            positive.push_back(cost);
        }
    }

    if (positive.empty()) {
        return text(TextId::CommonFree);
    }

    if (isCommonCost(positive)) {
        return format(TextId::BuildBarCostEach, positive.front().quantity);
    }

    std::string result;
    for (const events::CostEntry& cost : positive) {
        if (!result.empty()) {
            result += ' ';
        }
        result += format(TextId::BuildBarCostEntry, shortItemName(cost.item), cost.quantity);
    }
    return result;
}

} // namespace view::ui
