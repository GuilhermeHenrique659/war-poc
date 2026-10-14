#pragma once

#include "core/EventBus.hpp"
#include "model/Scene.hpp"
#include "view/DebugOverlay.hpp"
#include "view/MenuView.hpp"
#include "view/PlayingView.hpp"

#include <memory>

namespace view {

// Draws the frame for whichever scene the model last announced.
class Renderer {
public:
    explicit Renderer(core::EventBus& bus);

    void render() const;

private:
    model::Scene scene_ = model::Scene::MainMenu;
    std::unique_ptr<MenuView> menuView_;
    std::unique_ptr<PlayingView> playingView_;
    std::unique_ptr<DebugOverlay> debugOverlay_;
};

} // namespace view
