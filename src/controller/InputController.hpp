#pragma once

#include "controller/PlayingInputController.hpp"
#include "core/EventBus.hpp"
#include "model/Scene.hpp"

#include <cstddef>
#include <memory>
#include <optional>

namespace controller {

// Translates raw input into command events. Never mutates the model directly.
class InputController {
public:
    explicit InputController(core::EventBus& bus);

    void update();

private:
    void handleMenuKeyboard();
    void handleMenuMouse();
    [[nodiscard]] std::optional<std::size_t> menuItemUnderMouse() const;

    core::EventBus& bus_;
    model::Scene scene_ = model::Scene::MainMenu;
    std::size_t menuItemCount_ = 0;
    std::unique_ptr<PlayingInputController> playing_;
};

} // namespace controller
