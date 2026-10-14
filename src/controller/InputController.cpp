#include "controller/InputController.hpp"

#include "core/Events.hpp"
#include "view/MenuLayout.hpp"

#include <raylib.h>

namespace controller {

InputController::InputController(core::EventBus& bus)
    : bus_(bus)
    , playing_(std::make_unique<PlayingInputController>(bus))
{
    bus_.subscribe<events::SceneChanged>([this](const events::SceneChanged& e) { scene_ = e.scene; });
    bus_.subscribe<events::MenuStateChanged>(
        [this](const events::MenuStateChanged& e) { menuItemCount_ = e.labels.size(); });
}

void InputController::update()
{
    if (WindowShouldClose()) {
        bus_.publish(events::QuitRequested{});
        return;
    }
    if (IsKeyPressed(KEY_F3)) {
        bus_.publish(events::DebugToggleRequested{});
    }

    switch (scene_) {
    case model::Scene::MainMenu:
        handleMenuKeyboard();
        handleMenuMouse();
        return;
    case model::Scene::Playing:
        playing_->update();
        return;
    }
}

void InputController::handleMenuKeyboard()
{
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
        bus_.publish(events::MenuNavigateRequested{events::MenuDirection::Up});
        return;
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
        bus_.publish(events::MenuNavigateRequested{events::MenuDirection::Down});
        return;
    }
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_SPACE)) {
        bus_.publish(events::MenuConfirmRequested{});
    }
}

void InputController::handleMenuMouse()
{
    const auto hovered = menuItemUnderMouse();
    if (!hovered) {
        return;
    }

    // Only follow the mouse when it moves, so keyboard selection is not overridden.
    const Vector2 delta = GetMouseDelta();
    const bool moved = delta.x != 0.0f || delta.y != 0.0f;
    const bool clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    if (!moved && !clicked) {
        return;
    }

    bus_.publish(events::MenuItemHovered{*hovered});
    if (clicked) {
        bus_.publish(events::MenuConfirmRequested{});
    }
}

std::optional<std::size_t> InputController::menuItemUnderMouse() const
{
    const Vector2 mouse = GetMousePosition();
    for (std::size_t i = 0; i < menuItemCount_; ++i) {
        const Rectangle bounds = view::menu_layout::itemBounds(i, menuItemCount_, GetScreenWidth(), GetScreenHeight());
        if (CheckCollisionPointRec(mouse, bounds)) {
            return i;
        }
    }
    return std::nullopt;
}

} // namespace controller
