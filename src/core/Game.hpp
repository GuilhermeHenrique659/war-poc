#pragma once

#include "controller/InputController.hpp"
#include "core/EventBus.hpp"
#include "model/GameModel.hpp"
#include "view/Renderer.hpp"
#include "view/Window.hpp"

#include <memory>

namespace core {

// Composition root: owns every MVC piece and runs the main loop.
class Game {
public:
    Game();

    void run();

private:
    // Declaration order matters: the bus must outlive everything that subscribed to it,
    // and the window must outlive anything that uses the GL context.
    std::unique_ptr<EventBus> bus_;
    std::unique_ptr<view::Window> window_;
    std::unique_ptr<model::GameModel> model_;
    std::unique_ptr<view::Renderer> renderer_;
    std::unique_ptr<controller::InputController> controller_;
    bool running_ = true;
};

} // namespace core
