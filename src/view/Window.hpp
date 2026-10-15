#pragma once

#include <string>

namespace view {

// RAII owner of the raylib window and GL context.
class Window {
public:
    Window(int width, int height, const std::string& title);
    ~Window();

    void applyDisplayConfiguration(int width, int height, bool fullscreen) const;

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&) = delete;
    Window& operator=(Window&&) = delete;
};

} // namespace view
