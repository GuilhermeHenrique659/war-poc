#pragma once

#include "model/Scene.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace events {

// Controller -> Model (commands)
enum class MenuDirection { Up, Down };

struct MenuNavigateRequested {
    MenuDirection direction;
};

struct MenuItemHovered {
    std::size_t index;
};

struct MenuConfirmRequested {};

struct ReturnToMenuRequested {};

// Any -> Game loop
struct QuitRequested {};

// Model -> View / Controller (state notifications)
struct SceneChanged {
    model::Scene scene;
};

struct MenuStateChanged {
    std::vector<std::string> labels;
    std::size_t selectedIndex;
};

} // namespace events
