#pragma once

#include <array>
#include <cstddef>
#include <filesystem>
#include <format>
#include <string>
#include <string_view>

namespace view::ui {

// Every interface string is declared here once. The macro keeps the enum, the key
// table and the external catalog in sync.
#define WAR_TEXT_IDS(X) \
    X(CommonClose, "common.close") \
    X(CommonFree, "common.free") \
    X(CommonUnknown, "common.unknown") \
    X(MenuTitle, "menu.title") \
    X(MenuStart, "menu.start") \
    X(MenuSettings, "menu.settings") \
    X(MenuQuit, "menu.quit") \
    X(SettingsTitle, "settings.title") \
    X(SettingsResolution, "settings.resolution") \
    X(SettingsDisplayMode, "settings.display_mode") \
    X(SettingsPlayerName, "settings.player_name") \
    X(SettingsApply, "settings.apply") \
    X(SettingsBack, "settings.back") \
    X(SettingsFullscreen, "settings.fullscreen") \
    X(SettingsWindowed, "settings.windowed") \
    X(SettingsResolutionValue, "settings.resolution_value") \
    X(SettingsHint, "settings.hint") \
    X(SettingsDefaultPlayer, "settings.default_player") \
    X(HudPlayer, "hud.player") \
    X(HudHealth, "hud.health") \
    X(HudMonthWeek, "hud.month_week") \
    X(HudDayClock, "hud.day_clock") \
    X(HudCredit, "hud.credit") \
    X(HudInterest, "hud.interest") \
    X(BuildBarHelpShortcuts, "buildbar.help.shortcuts") \
    X(BuildBarHelpConstruct, "buildbar.help.construct") \
    X(BuildBarHelpCancel, "buildbar.help.cancel") \
    X(BuildBarHelpRecruit, "buildbar.help.recruit") \
    X(BuildBarHelpInventory, "buildbar.help.inventory") \
    X(BuildBarHelpSelectBuilding, "buildbar.help.select_building") \
    X(BuildBarHelpSelectUnits, "buildbar.help.select_units") \
    X(BuildBarHelpSelectUnitsAction, "buildbar.help.select_units_action") \
    X(BuildBarHelpMoveAttack, "buildbar.help.move_attack") \
    X(BuildBarHelpSupplies, "buildbar.help.supplies") \
    X(BuildBarHelpCruzador, "buildbar.help.cruzador") \
    X(BuildBarHelpMovePlayer, "buildbar.help.move_player") \
    X(BuildBarHelpZoom, "buildbar.help.zoom") \
    X(BuildBarSize, "buildbar.size") \
    X(BuildBarShortcut, "buildbar.shortcut") \
    X(BuildBarCostEach, "buildbar.cost_each") \
    X(BuildBarCostEntry, "buildbar.cost_entry") \
    X(BuildBarCredit, "buildbar.credit") \
    X(LockAlreadyBuilt, "lock.already_built") \
    X(LockRequiresCommandCenter, "lock.requires_command_center") \
    X(RecruitTitle, "recruit.title") \
    X(RecruitStats, "recruit.stats") \
    X(RecruitRangeSpeed, "recruit.range_speed") \
    X(RecruitDroneCapacity, "recruit.drone_capacity") \
    X(InventoryTitle, "inventory.title") \
    X(InventoryPlayer, "inventory.player") \
    X(InventoryUnits, "inventory.units") \
    X(InventoryPaneCount, "inventory.pane_count") \
    X(InventoryUnitsCount, "inventory.units_count") \
    X(InventoryTransferHint, "inventory.transfer_hint") \
    X(InventoryUnitsHint, "inventory.units_hint") \
    X(InventoryUnitsHintMore, "inventory.units_hint_more") \
    X(InventoryUnitHealth, "inventory.unit_health") \
    X(InventoryItemQuantity, "inventory.item_quantity") \
    X(LogisticsRecruitFree, "logistics.recruit_free") \
    X(LogisticsRecruitCost, "logistics.recruit_cost") \
    X(LogisticsEmptyWarehouse, "logistics.empty_warehouse") \
    X(LogisticsEmptySource, "logistics.empty_source") \
    X(LogisticsRoute, "logistics.route") \
    X(LogisticsRouteItems, "logistics.route_items") \
    X(LogisticsAssign, "logistics.assign") \
    X(LogisticsUnassign, "logistics.unassign") \
    X(TradeRow, "trade.row") \
    X(DebugFps, "debug.fps") \
    X(DebugScene, "debug.scene") \
    X(DebugCounts, "debug.counts") \
    X(DebugCamera, "debug.camera") \
    X(DebugMouse, "debug.mouse") \
    X(DebugBuilding, "debug.building") \
    X(DebugUnit, "debug.unit") \
    X(SceneMenu, "scene.menu") \
    X(SceneSettings, "scene.settings") \
    X(ScenePlaying, "scene.playing") \
    X(DroneConstruction, "drone.construction") \
    X(ItemIron, "item.iron") \
    X(ItemCoal, "item.coal") \
    X(ItemSteel, "item.steel") \
    X(ItemCopper, "item.copper") \
    X(ItemElectronics, "item.electronics") \
    X(ItemTools, "item.tools") \
    X(ItemWeapon, "item.weapon") \
    X(ItemAmmo, "item.ammo") \
    X(ItemShortIron, "item.short.iron") \
    X(ItemShortCoal, "item.short.coal") \
    X(ItemShortSteel, "item.short.steel") \
    X(ItemShortCopper, "item.short.copper") \
    X(ItemShortElectronics, "item.short.electronics") \
    X(ItemShortTools, "item.short.tools") \
    X(ItemShortWeapon, "item.short.weapon") \
    X(ItemShortAmmo, "item.short.ammo") \
    X(BuildingCommandCenter, "building.command_center") \
    X(BuildingWall, "building.wall") \
    X(BuildingHouse, "building.house") \
    X(BuildingGenerator, "building.generator") \
    X(BuildingWarehouse, "building.warehouse") \
    X(BuildingBarracks, "building.barracks") \
    X(BuildingCruzador, "building.cruzador") \
    X(BuildingTower, "building.tower") \
    X(BuildingIronMine, "building.iron_mine") \
    X(BuildingCoalMine, "building.coal_mine") \
    X(BuildingSteelworks, "building.steelworks") \
    X(BuildingCopperMine, "building.copper_mine") \
    X(BuildingElectronicsFactory, "building.electronics_factory") \
    X(BuildingToolFactory, "building.tool_factory") \
    X(BuildingWeaponFactory, "building.weapon_factory") \
    X(BuildingAmmoFactory, "building.ammo_factory") \
    X(BuildingTradeCenter, "building.trade_center") \
    X(BuildingTrainingDummy, "building.training_dummy") \
    X(UnitSoldier, "unit.soldier") \
    X(UnitSniper, "unit.sniper") \
    X(UnitEngineer, "unit.engineer") \
    X(UnitCombined, "unit.combined")

#define WAR_TEXT_ID_ENUM(name, key) name,
enum class TextId : std::size_t {
    WAR_TEXT_IDS(WAR_TEXT_ID_ENUM)
    Count
};
#undef WAR_TEXT_ID_ENUM

struct TextKey {
    TextId id;
    std::string_view key;
};

inline constexpr std::array<TextKey, static_cast<std::size_t>(TextId::Count)> kTextKeys{{
#define WAR_TEXT_ID_KEY(name, key) TextKey{TextId::name, key},
    WAR_TEXT_IDS(WAR_TEXT_ID_KEY)
#undef WAR_TEXT_ID_KEY
}};

[[nodiscard]] constexpr std::string_view textKey(TextId id)
{
    return kTextKeys[static_cast<std::size_t>(id)].key;
}

// Interface strings loaded from resource/texts/<language>.txt. Missing files,
// malformed lines, unknown keys, duplicates and missing keys are hard errors.
class TextCatalog {
public:
    static TextCatalog& instance();

    void load(std::string_view language);
    void loadFromFile(const std::filesystem::path& path);

    [[nodiscard]] const std::string& text(TextId id) const;
    [[nodiscard]] bool contains(TextId id) const;

    void clear();

private:
    std::array<std::string, static_cast<std::size_t>(TextId::Count)> texts_{};
    std::array<bool, static_cast<std::size_t>(TextId::Count)> present_{};
};

[[nodiscard]] const std::string& text(TextId id);

template <typename... Args>
[[nodiscard]] std::string format(TextId id, Args... args)
{
    return std::vformat(text(id), std::make_format_args(args...));
}

} // namespace view::ui
