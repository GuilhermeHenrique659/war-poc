#pragma once

#include "core/EventBus.hpp"
#include "model/Scene.hpp"
#include "view/menu/MenuView.hpp"
#include "view/menu/SettingsView.hpp"

namespace view::menu {

class MenuModule {
public:
    explicit MenuModule(core::EventBus& bus);

    void draw(model::Scene scene) const;

private:
    MenuView menuView_;
    SettingsView settingsView_;
};

} // namespace view::menu
