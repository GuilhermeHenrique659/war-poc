#pragma once

#include "core/EventBus.hpp"
#include "core/Events.hpp"
#include "model/BuildingType.hpp"
#include "model/Player.hpp"

#include <cstddef>
#include <optional>
#include <vector>

namespace view {

// Bottom bar listing the buildings the local player can select, plus the controls hint.
class BuildBarView {
public:
    explicit BuildBarView(core::EventBus& bus);

    void draw() const;

private:
    void drawSlot(std::size_t index) const;
    void drawHelp() const;

    std::vector<events::BuildBarEntry> entries_;
    std::optional<model::BuildingType> selected_;
    model::PlayerId localPlayerId_ = model::kNoPlayer;
};

} // namespace view
