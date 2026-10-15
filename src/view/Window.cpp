#include "view/Window.hpp"

#include <raylib.h>

namespace view {

Window::Window(int width, int height, const std::string& title)
{
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(width, height, title.c_str());
    SetTargetFPS(60);
    // ESC is handled by the controller; closing goes through the event bus.
    SetExitKey(KEY_NULL);
}

Window::~Window()
{
    CloseWindow();
}

void Window::applyDisplayConfiguration(int width, int height, bool fullscreen) const
{
    if (IsWindowFullscreen() && !fullscreen) {
        ToggleFullscreen();
    }
    SetWindowSize(width, height);
    if (!IsWindowFullscreen() && fullscreen) {
        ToggleFullscreen();
    }
}

} // namespace view
