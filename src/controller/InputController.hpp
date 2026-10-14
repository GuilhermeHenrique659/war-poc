#pragma once

#include "core/EventBus.hpp"
#include "model/Scene.hpp"

#include <cstddef>
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
    void handlePlayingInput();
    [[nodiscard]] std::optional<std::size_t> menuItemUnderMouse() const;

    core::EventBus& bus_;
    model::Scene scene_ = model::Scene::MainMenu;
    std::size_t menuItemCount_ = 0;
};

} // namespace controller
