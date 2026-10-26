#pragma once

#include "core/Events.hpp"
#include "model/Scene.hpp"
#include "model/build/BuildingType.hpp"
#include "model/economy/Item.hpp"
#include "model/menu/MenuAction.hpp"
#include "model/units/UnitType.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace view::ui {

// Domain enums to interface text. Views never read Model labels directly.
[[nodiscard]] std::string_view buildingName(model::BuildingType type);
[[nodiscard]] std::string_view unitName(model::UnitType type);
[[nodiscard]] std::string_view itemName(model::ItemId item);
[[nodiscard]] std::string_view shortItemName(model::ItemId item);
[[nodiscard]] std::string_view menuLabel(model::MenuAction action);
[[nodiscard]] std::string_view sceneName(model::Scene scene);
[[nodiscard]] std::string_view lockReason(events::BuildLockReason reason);

// Formats a material list, or the free label when costs are waived.
[[nodiscard]] std::string costText(const std::vector<events::CostEntry>& costs, bool free);

} // namespace view::ui
