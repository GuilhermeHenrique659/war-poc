#pragma once

#include "core/EventBus.hpp"
#include "view/BuildBarView.hpp"
#include "view/EconomyPanelView.hpp"
#include "view/MergePanelView.hpp"
#include "view/RecruitPanelView.hpp"
#include "view/WorldView.hpp"

#include <memory>

namespace view {

// Match scene: the isometric world with the build bar and the recruit / merge panels on top.
class PlayingView {
public:
    explicit PlayingView(core::EventBus& bus);

    void draw() const;

private:
    std::unique_ptr<WorldView> world_;
    std::unique_ptr<BuildBarView> buildBar_;
    std::unique_ptr<EconomyPanelView> economyPanel_;
    std::unique_ptr<RecruitPanelView> recruitPanel_;
    std::unique_ptr<MergePanelView> mergePanel_;
};

} // namespace view
