#pragma once

#include "core/EventBus.hpp"
#include "core/Events.hpp"

namespace view {

// Side panel shown while the local player has two or more units selected: previews the
// Combined unit and holds the merge button.
class MergePanelView {
public:
    explicit MergePanelView(core::EventBus& bus);

    void draw() const;

private:
    void drawStats() const;
    void drawButton() const;

    events::MergePanelChanged state_{};
};

} // namespace view
