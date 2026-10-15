#pragma once

#include "core/EventBus.hpp"

#include <cstddef>
#include <string>

namespace view {

class SettingsView {
public:
    explicit SettingsView(core::EventBus& bus);
    void draw() const;

private:
    std::size_t selectedIndex_ = 0;
    int width_ = 1280;
    int height_ = 720;
    bool fullscreen_ = false;
    std::string playerName_ = "Jogador";
};

} // namespace view
